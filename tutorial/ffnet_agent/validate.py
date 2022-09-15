#!/usr/bin/python3

from jsonschema import validate
import json

schema = json.load(open('nnet.schema.json'))
nnet = json.load(open('region_id_9.json'))

validate(instance=nnet, schema=schema)
