#!/usr/bin/env python

'''Connection methods for different databases.  The rest of the
interface should conform to PEP 249 (Python DB API), so only accessors
for the connection are needed.  Unfortunately, bound parameter string differs
between Maria and SQLite, so use '?' and replace if needed.
'''


class SQLiteDBConn:
    def __init__(self, filename):
        import sqlite3
        # TODO: if file does not exist, prompt for filename
        # use default location.  option to set a path instead?  then they would need to use it every time.
        self._conn = sqlite3.connect(filename)
        self._cursor = self._conn.cursor
        self._bind_str = '?'

    def conn(self):
        return self._conn

    def bind_str(self):
        return self._bind_str


class MariaDBConn:
    def __init__(self):
        import mysql
        import mysql.connector
        import getpass
        config = {
            'user': 'diana',
            'database': 'test',
        }
        password = getpass.getpass("Password:")
        self._conn = mysql.connector.connect(password=password, **config)
        self._conn.autocommit = False  # require commit() at end of transaction
        self._cursor = self._conn.cursor
        self._bind_str = '%s'

    def conn(self):
        return self._conn

    def bind_str(self):
        return self._bind_str
