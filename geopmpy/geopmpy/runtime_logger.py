#!/usr/bin/env python3
from flask import Flask, jsonify, request, abort, g
from argparse import ArgumentParser
import sqlite3

parser = ArgumentParser('Initialize (and clear if already existing) a geopmrt database')
parser.add_argument('database',
                    help='Path to a sqlite3 database target')
args = parser.parse_args()

app = Flask(__name__)

policy = dict(
    agent='monitor',
    sample_period=0.010,
    profile='filler profile name',
    params=dict(),
)

# See https://flask.palletsprojects.com/en/3.0.x/patterns/sqlite3/
# Recycles a DB connection if there's still an active one in the app context
def get_db():
    db = getattr(g, '_database', None)
    if db is None:
        db = g._database = sqlite3.connect(args.database)
    return db


def query_db(query, args=()):
    cur = get_db().execute(query, args)
    rv = cur.fetchall()
    column_names = [column[0]  # This is the actual column name. Other elements are unused by sqlite3.
                    for column in cur.description]
    cur.close()

    # Convert from rowwise to columnwise data.
    return dict(zip(
        column_names,
        list(zip(*rv))  # rv is a list of containing a tuple of values for each row. Transpose it.
    ))


@app.teardown_appcontext
def close_connection(exception):
    db = getattr(g, '_database', None)
    if db is not None:
        db.close()


@app.route('/policy', methods=['GET'])
def get_policy():
    return jsonify(policy)


@app.route('/reports', methods=['POST'])
def create_report():
    if not request.json:
        abort(400)
    db_conn = get_db()
    cur = db_conn.cursor()
    for report in request.json:
        cur.execute('INSERT INTO report(host, begin_sec, end_sec) '
                    'VALUES(?,?,?) '
                    'RETURNING report_id',
                    (report['host_url'], report['begin_sec'], report['end_sec']))
        report_row = cur.fetchone()
        (report_id, ) = report_row if report_row else None

        stat_insertions = [(report_id,
                            stat['name'], stat['count'],
                            stat['first'], stat['last'],
                            stat['min'], stat['max'],
                            stat['mean'], stat['std'])
                           for stat in report['stats']]
        cur.executemany('INSERT INTO stats(report_id, name, count, first, last, min, max, mean, std) '
                        'VALUES(?,?,?,?,?,?,?,?,?)',
                        stat_insertions)
        db_conn.commit()
    return '', 201


@app.route('/stats', methods=['GET'])
def get_stats_names():
    return jsonify(query_db('SELECT DISTINCT name FROM stats')['name'])


@app.route('/stats/<metric>', methods=['GET'])
def get_stats(metric):
    db_conn = get_db()
    cur = db_conn.cursor()

    limit = request.args.get("limit", None)
    since_start = request.args.get("since-start", None)

    query = ('SELECT s.*,r.begin_sec,r.end_sec '
             'FROM stats s JOIN report r '
             'ON r.report_id = s.report_id '
             'WHERE s.name=?')

    query_args = [metric]
    if since_start is not None:
        query += ' AND begin_sec > ?'
        query_args.append(since_start)

    if limit is not None:
        query += 'LIMIT ?'
        query_args.append(limit)

    stats = query_db(query, args=query_args)
    return jsonify({k: v for k, v in stats.items() if k not in ['name']})

if __name__ == '__main__':
    app.run(debug=True)
