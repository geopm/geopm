#!/usr/bin/env python3
import sqlite3
from argparse import ArgumentParser
import os

parser = ArgumentParser('Initialize (and clear if already existing) a geopmrt database')
parser.add_argument('database',
                    help='Path to a sqlite3 database target')
parser.add_argument('--schema',
                    default=os.path.join(os.path.dirname(os.path.abspath(__file__)), 'schema.sql'),
                    help='Path to the schema. Default: %(default)s')
args = parser.parse_args()

connection = sqlite3.connect(args.database)
connection.execute("PRAGMA foreign_keys = ON")

with open(args.schema) as f:
    connection.executescript(f.read())

connection.commit()
connection.close()
