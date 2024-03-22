#
#  Copyright (c) 2015 - 2024, Intel Corporation
#  SPDX-License-Identifier: BSD-3-Clause
#

import sys

"""Json schemas used by geopmdpy

GEOPM_ACTIVE_SESSIONS_SCHEMA:

Schema for describing an active GEOPM Service session.

GEOPM_SCHEMA_CONST_CONFIG_IO

Schema used describe a constant signal as used by the ConstConfigIOGroup.

"""

GEOPM_ACTIVE_SESSIONS_SCHEMA = """
    {
        "$schema": "http://json-schema.org/draft-04/schema#",
        "id": "https://geopm.github.io/active_sessions.schema.json",
        "title": "ActiveSession",
        "type": "object",
        "properties": {
          "client_pid": {
            "type": "integer"
          },
          "client_uid": {
            "type": "integer",
            "minimum": 0
          },
          "client_gid": {
            "type": "integer",
            "minimum": 0
          },
          "create_time": {
            "type": "number",
            "minimum": 0
	  },
          "reference_count": {
            "type": "integer",
            "minimum": 0
          },
          "signals": {
            "type": "array",
            "items": {
              "type": "string"
            }
          },
          "controls": {
            "type": "array",
            "items": {
              "type": "string"
            }
          },
          "watch_id": {
            "type": "integer"
          },
          "batch_server": {
            "type": "integer"
          },
          "profile_name": {
            "type": "string"
          }
        },
        "required": ["client_pid", "client_uid", "client_gid", "create_time", "signals", "controls"],
        "additionalProperties": false
    }
"""


GEOPM_CONST_CONFIG_IO_SCHEMA = """
    {
        "$schema": "http://json-schema.org/draft-04/schema#",
        "id": "https://geopm.github.io/const_config_io.schema.json",
        "title": "ConstConfigIOGroup config file specification",
        "definitions": {
            "signal_info": {
                "type": "object",
                "properties": {
                    "description": {
                        "type": "string"
                    },
                    "units": {
                        "enum": ["none", "seconds", "hertz", "watts", "joules", "celsius"]
                    },
                    "domain": {
                        "enum": ["board", "package", "core", "cpu", "memory", "package_integrated_memory", "nic", "package_integrated_nic", "gpu", "package_integrated_gpu", "gpu_chip"]
                    },
                    "aggregation": {
                        "enum": ["sum", "average", "median", "logical_and", "logical_or", "region_hash", "region_hint", "min", "max", "stddev", "select_first", "expect_same"]
                    },
                    "values": {
                        "type": "array",
                        "items": {
                            "type": "number"
                        },
                        "minItems": 1
                    },
                    "common_value": {
                        "type": "number"
                    }
                },
                "required": ["description", "units", "domain", "aggregation"],
                "oneOf": [
                    {"required": ["values"]},
                    {"required": ["common_value"]}
                ],
                "additionalProperties": false
            }
        },
        "type": "object",
        "additionalProperties": {
            "$ref": "#/definitions/signal_info"
        }
    }
"""

if __name__ == '__main__':
    usage = f'{sys.argv[0]} SCHEMA_DIRECTORY\n\n'
    if len(sys.argv) != 2:
        print(usage)
        sys.exit(0)

    schema_dir = sys.argv[1]
    with open(f'{schema_dir}/active_sessions.schema.json', 'w') as fid:
        fid.write(GEOPM_ACTIVE_SESSIONS_SCHEMA)
    with open(f'{schema_dir}/const_config_io.schema.json', 'w') as fid:
        fid.write(GEOPM_CONST_CONFIG_IO_SCHEMA)
