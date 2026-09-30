# MySQL connection pool recovery

## Behavior

- Idle connections are checked before reuse. Closed or disconnected sessions are
  destroyed and replaced; they are not put back into the idle queue.
- A failed connect or schema selection releases its reserved pool slot and wakes
  a waiter. Creating or validating a connection does not hold the pool mutex.
- Returning a connection validates it, rolls back an unfinished transaction and
  restores autocommit. Validation or reset failure discards the connection.
- Connector automatic reconnect is disabled. SQL callbacks and writes are never
  transparently replayed. A disconnect during COMMIT has an uncertain outcome;
  the caller must reconcile persisted state before deciding to retry.
- The pool must outlive all borrowed connections, as before.

## Optional configuration

Existing configuration files work without changes. These keys go inside `mysql`:

```json
{
  "port": 3306,
  "pool_size": 10,
  "pool_acquire_timeout_ms": 5000,
  "connect_timeout_seconds": 5,
  "read_timeout_seconds": 30,
  "write_timeout_seconds": 30
}
```

All limits must be positive; port must be 1-65535. The port is now explicitly
passed to the connector. Pool wait timeout applies to waiting for capacity, not
an end-to-end request deadline: connection/ping/SQL network operations are subject
to the connector's separate socket timeouts. Long-running queries may require a
higher read timeout. These timeouts do not prove whether a write was committed.

Warnings identify discarded idle connections and validation/reset failures.
Pool saturation raises `Timed out waiting for a MySQL pool connection` instead of
waiting indefinitely. Underlying connect/SQL failures still propagate to callers.
No credentials or SQL contents are logged by the pool.

## Verification

From the repository root:

```bash
cmake -S . -B build -DFETCHCONTENT_UPDATES_DISCONNECTED=ON
cmake --build build --target hubsql hubsql_tests -j 4
ctest --test-dir build --output-on-failure
```

The database integration test is skipped by default. To run it explicitly:

```bash
HUBSQL_DB_POOL_TEST_CONFIG=/home/wbl/hubsql/config/config.json \
  build/tests/hubsql_tests --gtest_filter='DbPoolIntegrationTest.*'
```

It opens dedicated test sessions, kills only a session created by the test, and
checks that the pool obtains a new usable connection. It does not restart MySQL,
kill HubSQL sessions, or modify persistent tables. Config credentials stay in
process memory and are not passed on the command line.

## Applying the fix

The new executable is `build/src/hubsql`. Rebuilding alone does not update an
already running process. Restart HubSQL using the environment's normal launcher
and the same config/UTXO directory after coordinating a service interruption.
MySQL does not need a restart, and no data reset or re-index is required.
