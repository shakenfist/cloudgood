#!/usr/bin/env python3

"""Drive a small write workload at a single Galera node.

All writes go to one node, which is how essentially every production Galera
deployment is configured -- see the "why OpenStack runs it this way" section
of the Galera chapter. The point of this script is not to be a benchmark; it
is to produce a steady, parallelisable stream of write sets so that the
effect of a slow node on the rest of the cluster becomes visible.

Deadlocks are counted rather than treated as errors, because in a Galera
cluster a commit time deadlock is a normal outcome rather than a fault.
"""

import argparse
import random
import string
import sys
import threading
import time

import pymysql


SCHEMA = 'flowdemo'

running = True
committed = 0
deadlocks = 0
counter_lock = threading.Lock()


def connect(host, port, password, database=None, timeout=10):
    return pymysql.connect(host=host, port=port, user='root', password=password, database=database,
                           autocommit=True, connect_timeout=timeout, read_timeout=timeout,
                           write_timeout=timeout)


def prepare(host, port, password, rows):
    """Create and populate the table we are going to hammer."""
    conn = connect(host, port, password)
    with conn.cursor() as cur:
        cur.execute('CREATE DATABASE IF NOT EXISTS %s' % SCHEMA)
        cur.execute('USE %s' % SCHEMA)
        cur.execute('DROP TABLE IF EXISTS load_test')

        # A primary key is not optional here. Galera uses the keys of the rows
        # a transaction touches to certify it against concurrent transactions
        # on other nodes, and a table without a primary key cannot be certified
        # reliably.
        cur.execute('CREATE TABLE load_test ('
                    '  id INT PRIMARY KEY,'
                    '  counter BIGINT NOT NULL DEFAULT 0,'
                    '  payload CHAR(200) NOT NULL'
                    ') ENGINE=InnoDB')

        filler = ''.join(random.choices(string.ascii_lowercase, k=200))
        cur.executemany('INSERT INTO load_test (id, counter, payload) VALUES (%s, 0, %s)',
                        [(i, filler) for i in range(rows)])
    conn.close()
    print('prepared %d rows in %s.load_test' % (rows, SCHEMA), file=sys.stderr)


def worker(host, port, password, rows):
    global committed, deadlocks

    conn = connect(host, port, password, database=SCHEMA)
    payload = ''.join(random.choices(string.ascii_lowercase, k=200))

    while running:
        try:
            with conn.cursor() as cur:
                cur.execute('UPDATE load_test SET counter = counter + 1, payload = %s WHERE id = %s',
                            (payload, random.randrange(rows)))
            with counter_lock:
                committed += 1
        except pymysql.Error as e:
            # 1213 is ER_LOCK_DEADLOCK, which on a Galera cluster also covers a
            # transaction that lost certification against another node. Which
            # pymysql exception class that arrives as has varied between
            # releases, so match on the error code rather than the type.
            if e.args and e.args[0] == 1213:
                with counter_lock:
                    deadlocks += 1
                continue
            if not running:
                break
            print('worker reconnecting after: %s' % (e,), file=sys.stderr)
            try:
                conn = connect(host, port, password, database=SCHEMA)
            except Exception:
                time.sleep(1)

    try:
        conn.close()
    except Exception:
        pass


def main():
    global running

    parser = argparse.ArgumentParser(description='Write load generator for the Galera flow control demo')
    parser.add_argument('--host', default='127.0.0.1')
    parser.add_argument('--port', type=int, default=3306, help='port of the node to write to (default node1)')
    parser.add_argument('--password', default='demopass')
    parser.add_argument('--threads', type=int, default=16)
    parser.add_argument('--rows', type=int, default=20000)
    parser.add_argument('--duration', type=int, default=0, help='seconds to run, or 0 for until interrupted')
    parser.add_argument('--skip-prepare', action='store_true')
    args = parser.parse_args()

    if not args.skip_prepare:
        prepare(args.host, args.port, args.password, args.rows)

    threads = []
    for _ in range(args.threads):
        t = threading.Thread(target=worker, args=(args.host, args.port, args.password, args.rows), daemon=True)
        t.start()
        threads.append(t)

    print('%d writer threads against %s:%d' % (args.threads, args.host, args.port), file=sys.stderr)
    print('%8s  %10s  %10s' % ('elapsed', 'commits/s', 'deadlocks'))

    started = time.time()
    last_committed, last_deadlocks = 0, 0
    try:
        while args.duration == 0 or time.time() - started < args.duration:
            time.sleep(1)
            with counter_lock:
                now_committed, now_deadlocks = committed, deadlocks
            print('%8.0f  %10d  %10d' % (time.time() - started,
                                         now_committed - last_committed,
                                         now_deadlocks - last_deadlocks))
            sys.stdout.flush()
            last_committed, last_deadlocks = now_committed, now_deadlocks
    except KeyboardInterrupt:
        pass
    finally:
        running = False
        for t in threads:
            t.join(timeout=3)

    print('total: %d commits, %d deadlocks' % (committed, deadlocks))


if __name__ == '__main__':
    main()
