/*
   Copyright (c) 2015 - 2024, Intel Corporation
   SPDX-License-Identifier: BSD-3-Clause 
*/

const express = require('express');
const sqlite3 = require('sqlite3');
const cors = require('cors');

const PORT = 3000;
const app = express();

//Point to database file
const db = new sqlite3.Database('/tmp/geopmrt.db')

var debug=0;
app.use(cors())

app.get('/report', (request, response) => {
	debug && console.log("Getting all reports");
	data_json = [];
	db.all("SELECT * FROM report",
		(error, rows) => {
			idx=0;
			rows.forEach((row) => {
				data_json[idx] = row;
				idx++
			});
			response.json(data_json);
		});
});

app.get('/report/:id', (request, response) => {
	report_id = request.params.id;
	debug && console.log("Getting report " + report_id);
	data_json = {}
	db.all("SELECT * FROM report where report_id=$id", {$id: report_id},
		(error, rows) => {
			debug && console.log(rows)
			rows.forEach((row) => {
				data_json[report_id] = row;
			});
			response.json(data_json);
		});
});

app.get('/times', (request, response) => {
	debug && console.log("Getting start and end times");
	db.all("SELECT MIN(begin_sec) AS start_time, MAX(end_sec) AS end_time FROM report WHERE begin_sec IS NOT 0 and begin_sec IS NOT NULL",
		(error, rows) => {
			response.json(rows[0]);
		});
});

app.get('/timeaxis/:name', (request, response) => {
	debug && console.log("Getting all timestamps");
	data_json = [];
	db.all("SELECT MIN(begin_sec) AS start_time, MAX(end_sec) AS end_time FROM report WHERE begin_sec IS NOT 0 AND begin_sec IS NOT NULL",
		(error, rows) => {
			start = rows[0].start_time;
			end = rows[0].end_time;
			db.all("SELECT begin_sec as time FROM stats LEFT JOIN report ON stats.report_id = report.report_id WHERE begin_sec IS NOT 0 AND name=$name", {$name: request.params.name},
				(error, rows) => {
					idx=0;
					debug && console.log(rows)
					rows.forEach((row) => {
						data_json[idx] = row.time-start
						idx++;
					});
			response.json(data_json);
				})
		});
});

app.get('/mean/:name', (request, response) => {
	debug && console.log("Getting all timestamps");
	data_json = [];
	db.all("SELECT mean FROM stats LEFT JOIN report ON stats.report_id = report.report_id WHERE name=$name AND begin_sec IS NOT 0", {$name: request.params.name},
		(error, rows) => {
			idx=0;
			debug && console.log(rows)
			rows.forEach((row) => {
				data_json[idx] = row.mean
				idx++;
			});
			response.json(data_json);
		})
});

app.get('/error/:name', (request, response) => {
	debug && console.log("Getting all errors");
	data_json = [];
	db.all("SELECT count, std FROM stats LEFT JOIN report ON stats.report_id = report.report_id WHERE name=$name AND count > 0 AND begin_sec IS NOT 0", {$name: request.params.name},
		(error, rows) => {
			idx=0;
			debug && console.log(rows)
			rows.forEach((row) => {
				data_json[idx] = 1.96 * row.std / Math.sqrt(row.count)
				idx++;
			});
			debug && console.log(data_json);
			response.json(data_json);
		})
});

app.get('/statsnames', (request, response) => {
	debug && console.log("Getting stats names");
	data_json = [];
	db.all("SELECT DISTINCT name FROM stats ",
		(error, rows) => {
			idx=0;
			debug && console.log(rows)
			rows.forEach((row) => {
				data_json[idx] = row.name;
				idx++;
			});
			response.json(data_json);
		});
});

app.get('/stats', (request, response) => {
	debug && console.log("Getting all stats");
	data_json = [];
	db.all("SELECT * FROM stats LEFT JOIN report ON stats.report_id = report.report_id WHERE mean IS NOT NULL AND begin_sec IS NOT 0",
		(error, rows) => {
			idx=0;
			debug && console.log(rows)
			rows.forEach((row) => {
				data_json[idx] = row;
				idx++;
			});
			response.json(data_json);
		});
});

app.listen(PORT, () => console.log(`GEOPM-RT Visualizer listening at http://localhost:${PORT}`));
