#!/usr/bin/env python3

"""Watch flow control across every node of a Galera cluster.

There is no log to tail for this. Galera writes nothing to the error log when
it engages flow control, nothing when the cluster pauses, and nothing when it
resumes, so the status variables are the only interface available.

A word on the counters, because the documentation and the implementation do
not agree. The MariaDB knowledge base describes wsrep_flow_control_sent,
wsrep_flow_control_recv and wsrep_local_recv_queue_avg as being measured
"since the most recent status query", which would mean reading them resets
them. Measured on MariaDB 11.4 that is not what happens: three consecutive
reads return identical values, the counters grow monotonically, and only
FLUSH STATUS resets them. So they are cumulative, and this script differences
consecutive samples itself rather than trusting the server to have done it.

That is also why wsrep_flow_control_paused_ns is used here in preference to
wsrep_flow_control_paused. The latter is a fraction of the time since the last
FLUSH STATUS, so on a server with months of uptime a severe stall averages
away to nothing. The nanosecond counter differenced over a known interval
gives the pause fraction for that interval instead, which is what an operator
actually wants to see.
"""

import argparse
import csv
import sys
import time

import pymysql


VARIABLES = ('wsrep_last_committed', 'wsrep_local_recv_queue', 'wsrep_flow_control_sent',
             'wsrep_flow_control_paused_ns', 'wsrep_local_state_comment', 'wsrep_cluster_size')

# Abbreviations for wsrep_local_state_comment, so a whole cluster's state fits
# in one column.
STATES = {'Synced': 'S', 'Donor/Desynced': 'D', 'Joined': 'J', 'Joiner': 'j', 'Initialized': 'I'}


class Node(object):
    def __init__(self, name, host, port, password):
        self.name = name
        self.host = host
        self.port = port
        self.password = password
        self.conn = None
        self.last_committed = None
        self.last_paused_ns = None

    def _connect(self):
        self.conn = pymysql.connect(host=self.host, port=self.port, user='root', password=self.password,
                                    autocommit=True, connect_timeout=3, read_timeout=3, write_timeout=3)

    def sample(self):
        """Return a dict of the current counters, or None if the node is unreachable."""
        try:
            if self.conn is None:
                self._connect()
            with self.conn.cursor() as cur:
                cur.execute('SHOW GLOBAL STATUS WHERE Variable_name IN (%s)'
                            % ','.join(['%s'] * len(VARIABLES)), VARIABLES)
                return {k: v for k, v in cur.fetchall()}
        except Exception:
            try:
                if self.conn:
                    self.conn.close()
            except Exception:
                pass
            self.conn = None
            return None


def main():
    parser = argparse.ArgumentParser(description='Flow control watcher for a Galera cluster')
    parser.add_argument('--host', default='127.0.0.1')
    parser.add_argument('--ports', default='3306,3307,3308', help='comma separated node ports')
    parser.add_argument('--names', default='node1,node2,node3')
    parser.add_argument('--password', default='demopass')
    parser.add_argument('--interval', type=float, default=1.0)
    parser.add_argument('--csv', help='also write samples to this file')
    args = parser.parse_args()

    ports = [int(p) for p in args.ports.split(',')]
    names = args.names.split(',')
    nodes = [Node(n, args.host, p, args.password) for n, p in zip(names, ports)]

    writer = None
    if args.csv:
        handle = open(args.csv, 'w', newline='')
        writer = csv.writer(handle)
        writer.writerow(['elapsed', 'commits_per_s'] +
                        ['%s_%s' % (n.name, f) for n in nodes for f in ('recvq', 'fc_sent', 'state')])

    header = ('%8s  %10s' % ('elapsed', 'commit/s') +
              ''.join(['  %8s' % ('%s q/fc' % n.name) for n in nodes]) +
              '  %8s  %7s' % ('paused%', 'states'))

    started = time.time()
    row = 0
    while True:
        samples = [n.sample() for n in nodes]
        elapsed = time.time() - started

        # Cluster commit rate, taken from the first reachable node. Every node
        # sees every write set, so any of them can tell us the cluster's rate.
        commits = '-'
        paused = '-'
        for node, sample in zip(nodes, samples):
            if sample is None:
                continue
            seqno = int(sample['wsrep_last_committed'])
            paused_ns = int(sample['wsrep_flow_control_paused_ns'])
            if node.last_committed is not None:
                commits = '%d' % int((seqno - node.last_committed) / args.interval)
                paused = '%.1f' % (100.0 * (paused_ns - node.last_paused_ns) / (args.interval * 1e9))
            node.last_committed = seqno
            node.last_paused_ns = paused_ns
            break

        cells = []
        states = []
        for sample in samples:
            if sample is None:
                cells.append('%8s' % '-/-')
                states.append('?')
                continue
            cells.append('%8s' % ('%s/%s' % (sample['wsrep_local_recv_queue'], sample['wsrep_flow_control_sent'])))
            states.append(STATES.get(sample['wsrep_local_state_comment'], '?'))

        if row % 20 == 0:
            print(header)
        print('%8.0f  %10s' % (elapsed, commits) + ''.join(['  %s' % c for c in cells]) +
              '  %8s  %7s' % (paused, '/'.join(states)))
        sys.stdout.flush()

        if writer:
            flat = []
            for sample in samples:
                if sample is None:
                    flat += ['', '', '']
                else:
                    flat += [sample['wsrep_local_recv_queue'], sample['wsrep_flow_control_sent'],
                             sample['wsrep_local_state_comment']]
            writer.writerow(['%.1f' % elapsed, commits] + flat)
            handle.flush()

        row += 1
        time.sleep(args.interval)


if __name__ == '__main__':
    try:
        main()
    except KeyboardInterrupt:
        pass
