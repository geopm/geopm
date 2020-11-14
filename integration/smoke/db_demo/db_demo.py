import flask
from markupsafe import escape
#import mariadb
import json
import mysql.connector
import getpass
import pandas

app = flask.Flask(__name__)

@app.route('/')
def index():
    return 'Index page'

@app.route('/hello')
def hello_world():
    return 'Hello, World!'

@app.route('/id/<int:uid>')
def show_data(uid):
    return 'ID = %s' % escape(uid)


config = {
    'user': 'diana',
    'database': 'test',
    }

password = getpass.getpass("Password:")

@app.route('/smoke')
def smoke():
    conn = mysql.connector.connect(password=password, **config)
    cur = conn.cursor()
    cur.execute("select * from RunResults")

    print(cur.description)
    rv = cur.fetchall()
    print(rv)

    return '{}'.format(rv)
    

with app.test_request_context():
    print(flask.url_for('index'))
    print(flask.url_for('hello_world'))
    print(flask.url_for('show_data', uid=776))

    # static files must be in static/
    print(flask.url_for('static', filename='style.css'))


