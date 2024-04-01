/*
   Copyright (c) 2015 - 2024, Intel Corporation
   SPDX-License-Identifier: BSD-3-Clause 
*/

function refreshGraphs(){
	if($('#auto').is(":checked")){
		$(".dograph").each(function(){
			check_id = $(this).id
			if($(this).is(":checked")){
				//Remove the "do_"
				stats_name = $(this).attr('id').substring(3);
				getAxes(stats_name,idToStatName(stats_name), function(){});
			}
		});
	}
}

function idName(statsname){
	var id_name = statsname.replace(" (", "_").replace(")","");
	return id_name;
}

function idToStatName(name){
	var stats_name = name.replace("_", " (").concat(")");
	return stats_name
}

function toggleGraph(stats_name){
	var graph_id = document.getElementById('graphs');
	//Populate graph when checkbox is checked
	if($('#do_' + stats_name).is(":checked")){
		console.log(stats_name + " is checked");
		graph_id.innerHTML +=  '<div id="' + stats_name + '" class="graph"></div>'
		getAxes(stats_name,idToStatName(stats_name), function(){});
	}
	//Remove graph when checkbox is unchecked
	else{
		console.log(stats_name + " is NOT checked");
		document.getElementById(stats_name).remove();
	}
}

function graphList(){
	var div_id = document.getElementById('runform');
	var content = "<table><tr><th></th><th>&nbsp;&nbsp;<img src='/images/graph_inv.png' width=30px></th></tr>\n"
	fetch('http://saruman.ra.intel.com:3000/statsnames')
		.then(response => response.json())
		.then(statsnames => {
			for(name in statsnames){
				id_name = idName(statsnames[name]);
				content += "<tr><td>" + statsnames[name] + "</td>" +
					"<td><input class='dograph' type=\"checkbox\" id=\"do_" + id_name + "\" onclick=toggleGraph(\'" + id_name + "\') checked ></td></tr>\n"
			}
			content += "</table>"
			div_id.innerHTML += content;

			for(name in statsnames){
				id_name = idName(statsnames[name]);
				toggleGraph(id_name);
			}
		});

}

function makeErrorAxes(x, y, error){
	var error_bars = {"x":[], "y":[]};
	var len = error.length;
	for(let idx=0; idx < len; idx++){
		error_bars.x.push(x[idx]);
		error_bars.y.push(error[idx]+y[idx]);
	}
	for(let idx=len-1; idx>=0; idx--){
		error_bars.x.push(x[idx]);
		error_bars.y.push(y[idx] - error[idx]);
	}
	return error_bars;
}

function drawGraph(eid, signal_name, x, y, error){
	var div_id = document.getElementById(eid);

	error_bars = makeErrorAxes(x, y, error);
	var errorbars = {
		x: error_bars.x,
		y: error_bars.y,
		fill: "tozerox",
		fillcolor: "rgba(0, 100, 80, 0.2)",
		line: {color: "transparent"},
		showlegend: false,
		type: "scatter"
	};
	var data = {
		x: x,
		y: y,
		line: {color: "rgb(0, 100, 80)"},
		mode: "lines",
		name: signal_name,
		type: "scatter"
	};

	var graph_data = [errorbars, data];

	var layout = {
		title: signal_name,
		xaxis: {
			title: "Time (s)",
		},
		yaxis: {
			title: signal_name,
		}
	};
	Plotly.newPlot(div_id, graph_data, layout);
}

function getTimeValues(eid, mean, error, signal_name, callback){
	fetch('http://saruman.ra.intel.com:3000/timeaxis/' + signal_name)
		.then(response => response.json())
		.then(time_axis => {
			drawGraph(eid, signal_name, time_axis, mean, error);
		});
}

function getErrorBars(eid, mean, signal_name, callback){
	fetch('http://saruman.ra.intel.com:3000/error/' + signal_name)
		.then(response => response.json())
		.then(error => {
			getTimeValues(eid, mean, error, signal_name, callback);
		});
}

function getAxes(eid, signal_name, callback){
	fetch('http://saruman.ra.intel.com:3000/mean/' + signal_name)
		.then(response => response.json())
		.then(mean => {
			getErrorBars(eid, mean, signal_name, callback);
		});
}
