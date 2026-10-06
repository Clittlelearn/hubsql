#!/usr/bin/env python3
"""Audit or initialize HubSQL MySQL tables without destructive migrations."""
import argparse
import ast
import getpass
import hashlib
import json
import os
from pathlib import Path
import re
import ssl
import sys
from decimal import Decimal

import pymysql
import sqlglot
from sqlglot import exp

ROOT = Path(__file__).resolve().parents[1]
SEED_SQL = "INSERT IGNORE INTO sync_status (id,last_synced_height,status) VALUES (1,0,'running')"


def identifier(value):
    if not re.fullmatch(r'[A-Za-z][A-Za-z0-9_]{0,63}', value):
        raise ValueError('Database/table identifiers must be 1-64 ASCII letters, digits or underscores')
    return '`' + value + '`'


def normalized_type(value):
    value = re.sub(r'\s+', '', value.lower())
    return re.sub(r'^(tinyint|smallint|mediumint|int|bigint)\(\d+\)', r'\1', value)


def default_value(value, data_type):
    if value is None:
        return None
    value = str(value)
    if value.lower() in ('current_timestamp', 'current_timestamp()'):
        return 'CURRENT_TIMESTAMP'
    if data_type.startswith('decimal'):
        return str(Decimal(value).normalize())
    return value


def table_contract(create):
    table = create.this
    if not isinstance(table, exp.Schema) or not isinstance(table.this, exp.Table):
        raise ValueError('Expected CREATE TABLE with an explicit column list')
    name = table.this.name
    identifier(name)
    if table.this.db or table.this.catalog or not create.args.get('exists'):
        raise ValueError('Schema must use unqualified CREATE TABLE IF NOT EXISTS')
    columns, indexes = {}, {}
    for item in table.expressions:
        if isinstance(item, exp.ColumnDef):
            dtype = normalized_type(item.args['kind'].sql(dialect='mysql'))
            column = dict(type=dtype, nullable=True, default=None, auto_increment=False, on_update=False)
            for constraint in item.args.get('constraints', []):
                kind = constraint.args['kind']
                if isinstance(kind, exp.PrimaryKeyColumnConstraint):
                    indexes['PRIMARY'] = dict(unique=True, columns=[item.name])
                    column['nullable'] = False
                elif isinstance(kind, exp.NotNullColumnConstraint):
                    column['nullable'] = bool(kind.args.get('allow_null'))
                elif isinstance(kind, exp.DefaultColumnConstraint):
                    node = kind.this
                    if isinstance(node, exp.Null):
                        value = None
                    elif isinstance(node, exp.Literal):
                        value = node.this
                    elif isinstance(node, exp.CurrentTimestamp):
                        value = 'CURRENT_TIMESTAMP'
                    else:
                        raise ValueError('Unsupported default expression: ' + node.sql())
                    column['default'] = default_value(value, dtype)
                elif isinstance(kind, exp.AutoIncrementColumnConstraint):
                    column['auto_increment'] = True
                elif isinstance(kind, exp.OnUpdateColumnConstraint):
                    if not isinstance(kind.this, exp.CurrentTimestamp):
                        raise ValueError('Only ON UPDATE CURRENT_TIMESTAMP is supported')
                    column['on_update'] = True
                elif not isinstance(kind, exp.CommentColumnConstraint):
                    raise ValueError('Unsupported column constraint: ' + kind.key)
            if item.name in columns:
                raise ValueError('Duplicate column: ' + item.name)
            columns[item.name] = column
        elif isinstance(item, exp.PrimaryKey):
            indexes['PRIMARY'] = dict(unique=True, columns=[c.name for c in item.expressions])
        elif isinstance(item, exp.UniqueColumnConstraint):
            indexes[item.this.name] = dict(unique=True, columns=[c.name for c in item.this.expressions])
        elif isinstance(item, exp.IndexColumnConstraint):
            indexes[item.name] = dict(unique=False, columns=[c.name for c in item.expressions])
        else:
            raise ValueError('Unsupported table definition: ' + item.key)
    for col in indexes.get('PRIMARY', {}).get('columns', []):
        columns[col]['nullable'] = False
    engines = [p.this.name for p in create.args['properties'].expressions if isinstance(p, exp.EngineProperty)]
    if engines != ['InnoDB']:
        raise ValueError('HubSQL schema requires InnoDB')
    return name, dict(columns=columns, indexes=indexes, engine='InnoDB')


def load_schema(root=ROOT):
    path = root / 'sql/schema.sql'
    text = path.read_text(encoding='utf-8-sig')
    tables, creates = {}, {}
    seed = sqlglot.parse_one(SEED_SQL, read='mysql').sql(dialect='mysql', comments=False)
    seen_seed = False
    for statement in sqlglot.parse(text, read='mysql'):
        if statement is None:
            continue
        if isinstance(statement, exp.Create) and statement.args.get('kind') == 'TABLE':
            name, contract = table_contract(statement)
            if name in tables:
                raise ValueError('Duplicate table definition: ' + name)
            tables[name] = contract
            creates[name] = (statement.sql(dialect='mysql', comments=False) +
                             ' DEFAULT CHARACTER SET utf8mb4 COLLATE utf8mb4_unicode_ci')
        elif isinstance(statement, exp.Insert) and statement.sql(dialect='mysql', comments=False) == seed:
            seen_seed = True
        else:
            raise ValueError('Unsafe/unsupported schema statement; migrations are never executed: ' + statement.key)
    if not tables or not seen_seed:
        raise ValueError('Schema must contain tables and the non-destructive sync_status seed')
    return tables, creates, hashlib.sha256(path.read_bytes()).hexdigest()


def source_tables(root=ROOT):
    # Extract C++ string literals first, then inspect SQL table references only.
    # This is a drift alarm, not a complete C++/dynamic SQL analyzer.
    references = {}
    strings = re.compile(r'//[^\n]*|/\*.*?\*/|R"([^ ()\\\t\r\n]{0,16})\((.*?)\)\1"|"(?:\\.|[^"\\])*"', re.S)
    table_pattern = re.compile(r'\b(?:FROM|JOIN|INTO|UPDATE)\s+`?([A-Za-z_][A-Za-z0-9_]*)', re.I)
    for path in sorted((root/'src').rglob('*')):
        if path.suffix not in ('.cpp', '.h'):
            continue
        values = []
        for match in strings.finditer(path.read_text(encoding='utf-8-sig')):
            token = match.group()
            if token.startswith(('//', '/*')):
                continue
            if token.startswith('R"'):
                values.append(match.group(2))
            else:
                try:
                    values.append(ast.literal_eval(token))
                except (SyntaxError, ValueError):
                    continue
        for value in values:
            if not re.match(r'^\s*(SELECT|INSERT|DELETE|UPDATE|REPLACE|FROM|JOIN)\b', value, re.I):
                continue
            for match in table_pattern.finditer(value):
                name = match.group(1)
                # ON DUPLICATE KEY UPDATE column=... is not a table reference.
                if match.group().upper().startswith('UPDATE') and re.search(r'KEY\s+$', value[:match.start()], re.I):
                    continue
                references.setdefault(name, set()).add(str(path.relative_to(root)))
    return {name: sorted(paths) for name, paths in sorted(references.items())}


def audit(root=ROOT):
    tables, creates, digest = load_schema(root)
    references = source_tables(root)
    missing = sorted(set(references) - tables.keys())
    portable = root/'deploy/mysql-portable/sql/schema.sql'
    mirror_matches = portable.read_bytes() == (root/'sql/schema.sql').read_bytes()
    result = dict(schema_sha256=digest, expected_tables=sorted(tables), table_count=len(tables),
                  source_references=references, source_tables_missing=missing, portable_schema_matches=mirror_matches)
    if missing or not mirror_matches:
        raise ValueError('Schema source audit failed: ' + json.dumps(result))
    return tables, creates, result


def rows(connection, sql, args=()):
    with connection.cursor() as cursor:
        cursor.execute(sql, args)
        return cursor.fetchall()


def execute(connection, sql, args=()):
    with connection.cursor() as cursor:
        cursor.execute(sql, args)


def inspect_database(connection, database):
    if not rows(connection, 'SELECT SCHEMA_NAME FROM information_schema.SCHEMATA WHERE SCHEMA_NAME=%s', (database,)):
        return None
    tables = {}
    for name, engine, collation, kind in rows(connection,
            'SELECT TABLE_NAME,ENGINE,TABLE_COLLATION,TABLE_TYPE FROM information_schema.TABLES WHERE TABLE_SCHEMA=%s', (database,)):
        tables[name] = dict(engine=engine, collation=collation, kind=kind, columns={}, indexes={})
    for name, column, dtype, nullable, default, extra in rows(connection,
            'SELECT TABLE_NAME,COLUMN_NAME,COLUMN_TYPE,IS_NULLABLE,COLUMN_DEFAULT,EXTRA '
            'FROM information_schema.COLUMNS WHERE TABLE_SCHEMA=%s ORDER BY TABLE_NAME,ORDINAL_POSITION', (database,)):
        dtype = normalized_type(dtype)
        tables[name]['columns'][column] = dict(type=dtype, nullable=nullable == 'YES',
            default=default_value(default, dtype), auto_increment='auto_increment' in extra.lower(),
            on_update='on update current_timestamp' in extra.lower())
    for name, index, non_unique, column, prefix, direction, visible in rows(connection,
            'SELECT TABLE_NAME,INDEX_NAME,NON_UNIQUE,COLUMN_NAME,SUB_PART,COLLATION,IS_VISIBLE '
            'FROM information_schema.STATISTICS WHERE TABLE_SCHEMA=%s ORDER BY TABLE_NAME,INDEX_NAME,SEQ_IN_INDEX', (database,)):
        entry = tables[name]['indexes'].setdefault(index, dict(unique=not non_unique, columns=[]))
        entry['columns'].append(column)
        if prefix is not None or direction != 'A' or visible != 'YES':
            entry['unsupported'] = True
    return tables


def compare(expected, actual):
    actual = actual or {}
    missing = sorted(expected.keys() - actual.keys())
    drift = []
    extra_columns = {}
    for name in sorted(expected.keys() & actual.keys()):
        want, have = expected[name], actual[name]
        if have['kind'] != 'BASE TABLE' or have['engine'] != want['engine']:
            drift.append(name + ': expected InnoDB base table')
        if not (have.get('collation') or '').startswith('utf8mb4_'):
            drift.append(name + ': expected utf8mb4 table charset')
        for column, contract in want['columns'].items():
            current = have['columns'].get(column)
            if current != contract:
                drift.append(f'{name}.{column}: expected {contract}, found {current}')
        for index, contract in want['indexes'].items():
            current = have['indexes'].get(index)
            if current != contract:
                drift.append(f'{name} index {index}: expected {contract}, found {current}')
        additions = sorted(have['columns'].keys() - want['columns'].keys())
        if additions:
            extra_columns[name] = additions
            for column in additions:
                c = have['columns'][column]
                if not c['nullable'] and c['default'] is None and not c['auto_increment']:
                    drift.append(f'{name}.{column}: extra required column may break application inserts')
        for index in have['indexes'].keys() - want['indexes'].keys():
            if have['indexes'][index]['unique']:
                drift.append(f'{name}: unexpected unique index {index} may reject valid inserts')
    return dict(missing_tables=missing, structure_drift=drift,
                extra_tables=sorted(actual.keys() - expected.keys()), extra_columns=extra_columns)


def initialize(connection, database, expected, creates, create_database=False):
    lock = 'hubsql-schema-' + hashlib.sha256(database.encode()).hexdigest()[:40]
    if rows(connection, 'SELECT GET_LOCK(%s, 10)', (lock,))[0][0] != 1:
        raise RuntimeError('Another schema initializer holds the database lock')
    try:
        actual = inspect_database(connection, database)
        before = compare(expected, actual)
        if before['structure_drift']:
            raise ValueError('Existing schema is incompatible; no changes made:\n' + '\n'.join(before['structure_drift']))
        if actual is None:
            if not create_database:
                raise ValueError('Database does not exist; use --create-database or ask the DBA to create it')
            execute(connection, 'CREATE DATABASE ' + identifier(database) +
                    ' CHARACTER SET utf8mb4 COLLATE utf8mb4_unicode_ci')
        connection.select_db(database)
        for name in before['missing_tables']:
            print('Creating table:', name, file=sys.stderr)
            execute(connection, creates[name])
        result = compare(expected, inspect_database(connection, database))
        if result['missing_tables'] or result['structure_drift']:
            raise ValueError('Post-create verification failed: ' + json.dumps(result))
        execute(connection, SEED_SQL)
        return before['missing_tables']
    finally:
        rows(connection, 'SELECT RELEASE_LOCK(%s)', (lock,))


def connect(args):
    config = {}
    if args.config:
        config = json.loads(args.config.read_text())['mysql']
    database = args.database or config.get('database', 'hubsql')
    identifier(database)
    host = args.host or config.get('host', '127.0.0.1')
    user = args.user or config.get('user')
    if not user:
        raise ValueError('--user or config.mysql.user is required')
    if args.prompt_password:
        password = getpass.getpass('MySQL password: ')
    else:
        password = os.environ.get('HUBSQL_DB_PASSWORD', config.get('password', ''))
    context = None
    if not args.socket and not args.allow_insecure:
        context = ssl.create_default_context(cafile=str(args.ssl_ca) if args.ssl_ca else None)
        context.minimum_version = ssl.TLSVersion.TLSv1_2
    connection = pymysql.connect(host=host, port=args.port or config.get('port', 3306), user=user,
        password=password, unix_socket=str(args.socket) if args.socket else None,
        ssl=context, ssl_disabled=context is None, charset='utf8mb4', autocommit=True, connect_timeout=10,
        read_timeout=60, write_timeout=60)
    if context and not rows(connection, "SHOW SESSION STATUS LIKE 'Ssl_cipher'")[0][1]:
        connection.close()
        raise ValueError('Server did not negotiate TLS; refusing an unencrypted TCP connection')
    version = rows(connection, 'SELECT VERSION()')[0][0]
    if 'mariadb' in version.lower() or int(version.split('.')[0]) < 8:
        connection.close()
        raise ValueError('This schema tool requires MySQL 8.0+ (not MariaDB)')
    return connection, database


def main(argv=None):
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument('action', choices=['audit', 'check', 'init'], nargs='?', default='audit')
    parser.add_argument('--host')
    parser.add_argument('--port', type=int)
    parser.add_argument('--user')
    parser.add_argument('--database')
    parser.add_argument('--socket', type=Path)
    parser.add_argument('--config', type=Path, help='Read the mysql object from an existing HubSQL config; never print it')
    parser.add_argument('--prompt-password', action='store_true')
    parser.add_argument('--ssl-ca', type=Path, help='CA bundle for TLS certificate AND hostname verification')
    parser.add_argument('--allow-insecure', action='store_true', help='Explicitly disable TLS for trusted local/test TCP only')
    parser.add_argument('--create-database', action='store_true', help='init only: create the database if absent')
    parser.add_argument('--report', type=Path, help='Write a JSON schema report (no credentials/row contents)')
    args = parser.parse_args(argv)
    if args.create_database and args.action != 'init':
        parser.error('--create-database requires init')
    if args.ssl_ca and args.allow_insecure:
        parser.error('--ssl-ca cannot be combined with --allow-insecure')
    expected, creates, result = audit()
    if args.action != 'audit':
        connection, database = connect(args)
        try:
            result['database'] = database
            if args.action == 'init':
                result['created_tables'] = initialize(connection, database, expected, creates, args.create_database)
            actual = inspect_database(connection, database)
            result['database_exists'] = actual is not None
            result['actual_tables'] = sorted(actual or {})
            result.update(compare(expected, actual))
            if 'sync_status' in (actual or {}) and not result['structure_drift']:
                result['sync_status_id_1_exists'] = bool(rows(connection,
                    'SELECT 1 FROM ' + identifier(database) + '.sync_status WHERE id=1'))
            result['ok'] = bool(result['database_exists'] and not result['missing_tables'] and
                                not result['structure_drift'] and result.get('sync_status_id_1_exists'))
        finally:
            connection.close()
    else:
        result['ok'] = True
    output = json.dumps(result, ensure_ascii=False, indent=2)
    print(output)
    if args.report:
        args.report.write_text(output + '\n', encoding='utf-8')
    return 0 if result['ok'] else 1


if __name__ == '__main__':
    try:
        sys.exit(main())
    except (ValueError, RuntimeError, OSError, pymysql.MySQLError, sqlglot.errors.SqlglotError) as error:
        print('ERROR:', error, file=sys.stderr)
        sys.exit(2)
