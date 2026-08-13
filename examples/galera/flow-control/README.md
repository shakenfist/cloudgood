# Galera flow control demonstration

This example builds a three node MariaDB Galera cluster in containers, drives
a write load at one node, and then deliberately makes a *different* node slow.
The point of the exercise is what happens to the node you did not touch.

It accompanies the [Galera and WSREP replication](../../../docs/galera.md)
chapter, and demonstrates the claim in its flow control section: one slow node
pauses the entire cluster.

## Requirements

Docker with Compose, and Python 3 with `pymysql`. Because most distributions
now enforce PEP 668, use a virtual environment:

```bash
python3 -m venv venv
./venv/bin/pip install pymysql
```

## Running it

```bash
docker compose up -d          # takes a minute; node2 and node3 each SST
PYTHON=./venv/bin/python ./demo.sh
```

`demo.sh` runs a healthy baseline, applies a throttle, removes it, and watches
the recovery, leaving the raw samples in `results/`. When you are finished:

```bash
docker compose down -v
```

## What it actually measured

The numbers below are a real run on a sixteen core host. Eight writer threads
are pointed at node1 only, which is how essentially every production Galera
deployment is configured. `q` is the node's receive queue depth and `fc` is
its cumulative count of flow control pause events sent.

Healthy, with all three nodes keeping up:

```text
 elapsed    commit/s  node1 q/fc  node2 q/fc  node3 q/fc   paused%   states
      17       13501       0/0     0/313     1/327       0.0    S/S/S
      18       13041       0/0     0/314     1/327       0.3    S/S/S
      19       12544       0/0     0/319     0/330       0.5    S/S/S
```

At twenty seconds node3 is told to fsync every applied transaction and to use
a single applier thread, which is what a node with slower storage looks like:

```text
      20       12049       0/0     0/320    23/330       0.0    S/S/S
      21        1082       0/0     0/320    32/464      94.1    S/S/S
      22        1066       0/0     0/320    31/597      93.0    S/S/S
      23        1044       0/0     0/320    28/727      92.6    S/S/S
      ...
      49         408       0/0     0/320   32/2421      97.5    S/S/S
      50         463       0/0     0/320   33/2478      97.5    S/S/S
```

Cluster write throughput falls from around 13,000 commits per second to around
500, a factor of roughly twenty five. The cluster spends 97% of its time
paused. node3 sends more than two thousand flow control events in thirty
seconds.

The most important column is the one that does nothing. **node1, the node
taking every single write, reports a receive queue of zero and has sent no
flow control at all.** An operator looking only at the writer sees a database
that has become twenty five times slower for no visible reason. The evidence
is entirely on a node nobody was watching.

Removing the throttle recovers immediately:

```text
      50         463       0/0     0/320   33/2478      97.5    S/S/S
      51       11827       0/0     0/320    0/2480       3.4    S/S/S
      52       15106       0/0     0/320    0/2480       0.0    S/S/S
```

## Two mechanisms, not one

The chapter separates commit latency, which is set by the worst network path,
from sustained throughput, which is set by the worst apply rate. The demo can
show either, and which one you get depends on how you cripple the node.

`MODE=storage` (the default) slows only the apply path, so the receive queue
grows, flow control engages, and you see the output above.

`MODE=cpu` starves the whole container instead. That also starves node3's
group communication threads, so the cluster degrades through the ordering path
rather than the apply path. The result looks quite different:

```text
      20       11836       0/0     0/129     0/130       2.4    S/S/S
      21        1249       0/0     0/129     0/131       0.0    S/S/S
      22        1302       0/0     0/129     1/132       0.0    S/S/S
```

Throughput collapses by a similar factor, but **the receive queues stay empty
and nothing is ever paused**. No amount of applier tuning would have helped
here. This is worth running precisely because it looks like the same fault and
is not, which is the practical value of the chapter's diagnostic rule: high
latency with empty queues means the network, a growing queue means apply.

## A note on the counters

The MariaDB knowledge base describes `wsrep_flow_control_sent`,
`wsrep_flow_control_recv` and `wsrep_local_recv_queue_avg` as measured "since
the most recent status query", which would mean that reading them resets them.
On MariaDB 11.4 that is not the observed behaviour:

```console
$ docker exec cg-node3 mariadb -uroot -pdemopass -N -B -e "
    SHOW GLOBAL STATUS WHERE Variable_name IN ('wsrep_flow_control_sent');
    DO SLEEP(4);
    SHOW GLOBAL STATUS WHERE Variable_name IN ('wsrep_flow_control_sent');
    FLUSH STATUS;
    SHOW GLOBAL STATUS WHERE Variable_name IN ('wsrep_flow_control_sent');"
wsrep_flow_control_sent 3979
wsrep_flow_control_sent 4003
wsrep_flow_control_sent 0
```

The counters are cumulative, they grow monotonically, and only `FLUSH STATUS`
clears them. `fcwatch.py` therefore differences consecutive samples itself.
Check this on your own version rather than trusting either the documentation
or this README.

## Files

`docker-compose.yml`
:   Three MariaDB nodes. node1 bootstraps with `--wsrep-new-cluster`; the
    other two join, which means each takes a state transfer on first start.

`conf/galera.cnf`
:   The cluster configuration, including the `gcs.fc_limit` that decides how
    deep a receive queue has to get before a node throttles everybody else.

`conf/healthcheck.sh`
:   Tests for `wsrep_local_state_comment` being `Synced`, because a Galera
    node that is listening but not Synced will refuse queries.

`loadgen.py`
:   Write load against a single node, reporting client observed throughput and
    counting commit time deadlocks separately from real errors.

`fcwatch.py`
:   Polls the flow control counters on every node once a second and prints
    them side by side. Also writes a CSV.

`demo.sh`
:   Runs the whole sequence: baseline, throttle, restore, recovery.
