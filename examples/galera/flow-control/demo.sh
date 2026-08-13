#!/bin/bash
#
# Run the flow control demonstration end to end.
#
# Starts a write load against node1 only, watches the flow control counters on
# all three nodes, and part way through deliberately makes node3 slow at
# applying write sets. The interesting result is what happens to node1, which
# we never touched.
#
# Two ways of crippling node3 are provided, and they do NOT demonstrate the
# same thing:
#
#   MODE=storage  (default)   Makes node3 fsync every applied transaction and
#                             gives it a single applier thread, which is what
#                             a node with slower disks looks like. This slows
#                             the apply path only, leaving group communication
#                             healthy, so node3's receive queue grows, it
#                             sends flow control, and the whole cluster is
#                             paused. This is the flow control demo.
#
#   MODE=cpu                  Starves the whole container of CPU. This also
#                             slows node3's group communication threads, so
#                             the cluster degrades through the *ordering* path
#                             instead: throughput collapses while receive
#                             queues stay empty and nothing is ever paused.
#                             Useful for showing that the two mechanisms in
#                             the chapter are genuinely different.
#
# The cluster must already be running: docker compose up -d

set -euo pipefail
cd "$(dirname "$0")"

PYTHON=${PYTHON:-python3}
NODE=${NODE:-cg-node3}          # the node we are going to cripple
MODE=${MODE:-storage}           # storage | cpu
CPUS=${CPUS:-0.15}              # MODE=cpu: how much CPU to leave it
THREADS=${THREADS:-8}           # writer threads against node1
BASELINE=${BASELINE:-20}        # seconds of healthy cluster before throttling
THROTTLED=${THROTTLED:-30}      # seconds spent throttled
RECOVERY=${RECOVERY:-25}        # seconds to watch it recover afterwards
OUT=${OUT:-results}

MYSQL="mariadb -uroot -pdemopass -N -B"
TOTAL=$((BASELINE + THROTTLED + RECOVERY))
mkdir -p "${OUT}"

# Remember the healthy applier thread count so we can put it back.
NORMAL_APPLIERS=$(docker exec "${NODE}" ${MYSQL} \
    -e "SELECT @@wsrep_slave_threads" 2>/dev/null | tr -d '[:space:]')
NORMAL_APPLIERS=${NORMAL_APPLIERS:-8}

throttle() {
    if [ "${MODE}" = 'cpu' ]; then
        echo "== starving ${NODE} of CPU (${CPUS} cores)"
        docker update --cpus "${CPUS}" "${NODE}" >/dev/null
    else
        echo "== giving ${NODE} slow storage and a single applier thread"
        docker exec "${NODE}" ${MYSQL} \
            -e 'SET GLOBAL innodb_flush_log_at_trx_commit = 1; SET GLOBAL wsrep_slave_threads = 1' >/dev/null
    fi
}

restore() {
    if [ "${MODE}" = 'cpu' ]; then
        # "--cpus 0" does NOT clear an existing limit, so put back something
        # at least as large as the host.
        docker update --cpus "$(nproc)" "${NODE}" >/dev/null 2>&1 || true
    else
        docker exec "${NODE}" ${MYSQL} \
            -e "SET GLOBAL innodb_flush_log_at_trx_commit = 2;
                SET GLOBAL wsrep_slave_threads = ${NORMAL_APPLIERS}" >/dev/null 2>&1 || true
    fi
}

cleanup() {
    restore
    kill "${LOAD_PID:-}" "${WATCH_PID:-}" >/dev/null 2>&1 || true
    wait "${LOAD_PID:-}" "${WATCH_PID:-}" 2>/dev/null || true
}
trap cleanup EXIT INT TERM

echo "== preparing table and starting ${THREADS} writer threads against node1"
"${PYTHON}" loadgen.py --threads "${THREADS}" --duration "$((TOTAL + 15))" \
    > "${OUT}/loadgen.txt" 2> "${OUT}/loadgen.err" &
LOAD_PID=$!

# Give the load a moment to get past table preparation before we start
# measuring, otherwise the first few samples are all schema creation.
sleep 12

echo "== watching flow control counters for ${TOTAL}s (mode: ${MODE})"
"${PYTHON}" fcwatch.py --csv "${OUT}/counters.csv" > "${OUT}/fcwatch.txt" 2>&1 &
WATCH_PID=$!

sleep "${BASELINE}"
throttle

sleep "${THROTTLED}"
echo "== removing the throttle from ${NODE}"
restore

sleep "${RECOVERY}"
echo "== done, results in ${OUT}/"

kill "${WATCH_PID}" >/dev/null 2>&1 || true
wait "${WATCH_PID}" 2>/dev/null || true

echo
echo "== flow control counters"
cat "${OUT}/fcwatch.txt"
