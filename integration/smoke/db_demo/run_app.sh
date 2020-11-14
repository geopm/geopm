#!/bin/bash

source venv/bin/activate

export FLASK_APP=db_demo.py

flask run --host=0.0.0.0 --port 8080
