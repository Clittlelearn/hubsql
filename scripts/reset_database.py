#!/usr/bin/env python3
"""Preview or rebuild this checkout's portable MySQL and paired UTXO store."""
import argparse
import fcntl
import json
import os
from pathlib import Path
import re
import secrets
import socket
import subprocess
import sys
import time
from datetime import datetime, timezone

import init_database as schema

CONFIRMATION = 'RESET-ALL-LOCAL-MYSQL'


def local_path(root, value):
    path = Path(os.path.abspath(root / value))
    if path == root or root not in path.parents:
        raise ValueError('Data paths must be inside the project root')
    for part in [path, *path.parents]:
        if part == root:
            break
        if part.is_symlink():
            raise ValueError('Symlink paths are not supported: ' + str(part))
    if any(char in str(path) for char in '\n\r\0'):
        raise ValueError('Invalid path characters')
    return path


def make_plan(root, config_path):
    root = root.resolve()
    config = json.loads(config_path.read_text(encoding='utf-8-sig'))
    mysql = config['mysql']
    if mysql.get('host', '127.0.0.1') not in ('localhost', '127.0.0.1'):
        raise ValueError('Only local portable MySQL is supported; remote/RDS reset is forbidden')
    database = mysql['database']
    schema.identifier(database)
    if database.lower() in ('mysql', 'sys', 'information_schema', 'performance_schema'):
        raise ValueError('Application database cannot be a MySQL system schema')
    user = mysql['user']
    if not re.fullmatch(r'[A-Za-z0-9_]{1,32}', user):
        raise ValueError('Application MySQL user must contain 1-32 letters, digits or underscores')
    if not isinstance(mysql.get('password'), str) or not mysql['password']:
        raise ValueError('Set a non-empty mysql.password in the application config before resetting')
    port = mysql.get('port', 3306)
    if type(port) is not int or not 1 <= port <= 65535:
        raise ValueError('Invalid mysql.port')
    if config.get('chain', {}).get('start_height', 0) != 0:
        raise ValueError('Set chain.start_height=0 for a paired full reindex before resetting')
    base = local_path(root, 'deploy/mysql-portable')
    data = local_path(root, 'deploy/mysql-portable/data')
    run = local_path(root, 'deploy/mysql-portable/run')
    cnf = local_path(root, 'deploy/mysql-portable/config/my.cnf')
    utxo = local_path(root, config.get('rocksdb', {}).get('path', './data/utxo'))
    # Limit resets to runtime data, never source/config/dependency directories.
    runtime_root = local_path(root, 'data')
    if runtime_root not in utxo.parents:
        raise ValueError('rocksdb.path must be a subdirectory of the project data/ directory')
    for directory in (data, run, utxo):
        if directory.exists() and not directory.is_dir():
            raise ValueError('Expected directory: ' + str(directory))
    binary = base/'bin/mysqld'
    if not binary.is_file() or not os.access(binary, os.X_OK):
        raise ValueError('Portable mysqld binary is missing; install MySQL first')
    expected, creates, audit = schema.audit(root)
    return dict(root=root, base=base, data=data, run=run, cnf=cnf, utxo=utxo,
                binary=binary, mysql=mysql, expected=expected, creates=creates, audit=audit)


def assert_stopped(port):
    found = []
    for proc in Path('/proc').iterdir():
        if not proc.name.isdigit():
            continue
        try:
            name = (proc/'comm').read_text().strip().lower()
        except FileNotFoundError:
            continue
        if name in ('mysqld', 'mysqld_safe', 'hubsql') or name.startswith('hubsql_'):
            found.append(proc.name + ':' + name)
    if found:
        raise ValueError('Stop HubSQL/MySQL first; running processes: ' + ', '.join(found))
    with socket.socket() as probe:
        probe.bind(('127.0.0.1', port))


def private_write(path, text):
    with path.open('x', encoding='utf-8') as output:
        os.chmod(path, 0o600)
        output.write(text)


def wait_connection(process, socket_path, password='', timeout=60):
    until = time.monotonic() + timeout
    while time.monotonic() < until:
        if process.poll() is not None:
            raise RuntimeError('Bootstrap MySQL exited; inspect the saved reset logs')
        if not socket_path.exists():
            time.sleep(0.5)
            continue
        connection = schema.pymysql.connect(unix_socket=str(socket_path), user='root', password=password,
                                            autocommit=True, charset='utf8mb4', ssl_disabled=True,
                                            connect_timeout=3, read_timeout=30, write_timeout=30,
                                            defer_connect=True)
        try:
            connection.connect()
            return connection
        except schema.pymysql.MySQLError:
            connection.close()
            time.sleep(0.5)
    raise RuntimeError('Bootstrap MySQL timed out; inspect the saved reset logs')


def reset(plan):
    base, root, run = plan['base'], plan['root'], plan['run']
    backups = local_path(root, '.database-reset-backups')
    backups.mkdir(mode=0o700, exist_ok=True)
    os.chmod(backups, 0o700)
    with (backups/'reset.lock').open('a') as lock:
        fcntl.flock(lock, fcntl.LOCK_EX | fcntl.LOCK_NB)
        assert_stopped(plan['mysql'].get('port', 3306))
        backup = backups / (datetime.now(timezone.utc).strftime('%Y%m%dT%H%M%SZ') + '-' + secrets.token_hex(4))
        backup.mkdir(mode=0o700)
        print('Backup directory:', backup, flush=True)
        # Record the complete plan before moving anything; failures are not auto-rolled back.
        manifest = {key: str(plan[key]) for key in ('data', 'run', 'utxo', 'cnf')}
        private_write(backup/'paths.json', json.dumps(manifest, indent=2) + '\n')
        for key in ('data', 'run', 'utxo', 'cnf'):
            source = plan[key]
            if source.exists():
                source.rename(backup/key)
        run.mkdir(mode=0o700, parents=True)
        plan['data'].mkdir(mode=0o700)
        plan['cnf'].parent.mkdir(parents=True, exist_ok=True)
        mysql = plan['mysql']
        root_password = mysql['password'] if mysql['user'] == 'root' else secrets.token_urlsafe(32)
        # Keep admin access recoverable even if a later step fails; never print the secret.
        private_write(backup/'root-client.cnf', '[client]\nuser=root\npassword=' + json.dumps(root_password, ensure_ascii=False) +
                      '\nsocket=' + json.dumps(str(run/'mysqld.sock')) + '\n')
        cnf = ('[mysqld]\n' + '\n'.join(f'{key}={json.dumps(str(value))}' for key, value in {
            'basedir': base, 'datadir': plan['data'], 'socket': run/'mysqld.sock',
            'pid-file': run/'mysqld.pid', 'log-error': run/'error.log',
        }.items()) + f'\nport={mysql.get("port", 3306)}\nbind-address=127.0.0.1\n'
               'mysqlx=OFF\ncharacter-set-server=utf8mb4\ncollation-server=utf8mb4_unicode_ci\n'
               'max_connections=200\ninnodb_buffer_pool_size=512M\n')
        private_write(plan['cnf'], cnf)
        with (run/'reset-initialize.log').open('x') as log:
            subprocess.run([str(plan['binary']), '--no-defaults', '--initialize-insecure',
                            '--basedir=' + str(base), '--datadir=' + str(plan['data'])],
                           stdout=log, stderr=log, check=True)
        with (run/'reset-bootstrap.log').open('x') as log:
            process = subprocess.Popen([str(plan['binary']), '--defaults-file=' + str(plan['cnf']),
                                        '--skip-networking'], stdout=log, stderr=log)
            connection = None
            try:
                connection = wait_connection(process, run/'mysqld.sock')
                schema.execute(connection, "ALTER USER 'root'@'localhost' IDENTIFIED BY %s", (root_password,))
                schema.initialize(connection, mysql['database'], plan['expected'], plan['creates'], True)
                if mysql['user'] != 'root':
                    for host in ('localhost', '127.0.0.1'):
                        schema.execute(connection, 'CREATE USER %s@%s IDENTIFIED BY %s',
                                       (mysql['user'], host, mysql['password']))
                        schema.execute(connection, 'GRANT SELECT,INSERT,UPDATE,DELETE ON ' +
                                       schema.identifier(mysql['database']) + '.* TO %s@%s', (mysql['user'], host))
                with schema.pymysql.connect(unix_socket=str(run/'mysqld.sock'), user=mysql['user'],
                                            password=mysql['password'], database=mysql['database'],
                                            charset='utf8mb4', ssl_disabled=True) as app:
                    diff = schema.compare(plan['expected'], schema.inspect_database(app, mysql['database']))
                    if diff['missing_tables'] or diff['structure_drift']:
                        raise RuntimeError('Application account schema verification failed')
                    if schema.rows(app, 'SELECT last_synced_height FROM sync_status WHERE id=1') != ((0,),):
                        raise RuntimeError('Fresh sync height is not zero')
                private_write(backup/'result.json', json.dumps(dict(ok=True, tables=len(plan['expected']),
                              database=mysql['database'], schema_sha256=plan['audit']['schema_sha256']), indent=2))
            finally:
                if connection:
                    try:
                        schema.execute(connection, 'SHUTDOWN')
                    except schema.pymysql.MySQLError:
                        pass
                    connection.close()
                if process.poll() is None:
                    process.terminate()
                try:
                    process.wait(timeout=60)
                except subprocess.TimeoutExpired:
                    raise RuntimeError('Bootstrap MySQL did not stop; no SIGKILL sent. Do not restart other services.')
        print('RESET COMPLETE. MySQL is stopped; UTXO will be recreated by HubSQL.')
        print('Admin credentials (mode 600):', backup/'root-client.cnf')
        print('Start MySQL: bash ' + str(base/'scripts/start.sh'))
        print('Then start HubSQL from:', root)


def main(argv=None):
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument('--config', type=Path, default=schema.ROOT/'config/config.json')
    parser.add_argument('--execute', action='store_true', help='Move old data aside and initialize a fresh local instance')
    parser.add_argument('--services-stopped', action='store_true', help='Confirm HubSQL, MySQL and watchdog are stopped')
    parser.add_argument('--confirm', help='Required with --execute: ' + CONFIRMATION)
    args = parser.parse_args(argv)
    if sys.platform != 'linux':
        raise ValueError('Run on Linux or inside WSL')
    plan = make_plan(schema.ROOT, args.config)
    print('Scope: ENTIRE portable MySQL instance (all databases/accounts) AND paired HubSQL UTXO.')
    for key in ('data', 'run', 'utxo', 'cnf'):
        print(key + ':', plan[key])
    print('Restore application database:', plan['mysql']['database'], '; tables:', len(plan['expected']))
    print('Old directories are renamed into .database-reset-backups, never deleted. No config passwords are printed.')
    if not args.execute:
        print('PREVIEW ONLY. Nothing changed.')
        return 0
    if not args.services_stopped or args.confirm != CONFIRMATION:
        raise ValueError('Execution requires --services-stopped --confirm ' + CONFIRMATION)
    if os.geteuid() == 0:
        raise ValueError('Run as the non-root account owning this project, not sudo/root')
    os.umask(0o077)
    reset(plan)
    return 0


if __name__ == '__main__':
    try:
        sys.exit(main())
    except schema.pymysql.MySQLError as error:
        print('FAILED: MySQL error code', error.args[0], '(details withheld to protect credentials). Inspect reset logs.', file=sys.stderr)
        sys.exit(1)
    except (ValueError, RuntimeError, OSError, KeyError, subprocess.SubprocessError) as error:
        print('FAILED:', error, '\nNo automatic rollback. Preserve the backup and inspect before retrying.', file=sys.stderr)
        sys.exit(1)
