#!/bin/sh
# A Galera node is not useful merely because mariadbd is listening. Until it
# has finished its state transfer and reached Synced it will refuse queries
# with "WSREP has not yet prepared node for application use", so that is what
# we actually test for here.
state=$(MYSQL_PWD="${MARIADB_ROOT_PASSWORD}" mariadb -uroot -N -B \
        -e "SHOW STATUS LIKE 'wsrep_local_state_comment'" 2>/dev/null | awk '{print $2}')
[ "${state}" = 'Synced' ]
