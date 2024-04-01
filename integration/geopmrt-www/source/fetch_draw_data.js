/*
   Copyright (c) 2015 - 2024, Intel Corporation
   SPDX-License-Identifier: BSD-3-Clause 
*/

/////////////////////
// Manage Events  //
///////////////////

/*---List all available stats on page load---*/
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

/*---Refresh all graphs periodically when Auto-Refresh is Checked---*/
function autoRefreshGraphs(){
	if($('#auto').is(":checked")){
		refreshGraphs()
	}
}

/*---Graph stats that are checked---*/
function refreshGraphs(){
	$(".dograph").each(function(){
		check_id = $(this).id
		if($(this).is(":checked")){
			//Remove the "do_"
			stats_name = $(this).attr('id').substring(3);
			//TODO: This call might be the problem
			graphStat(stats_name);
		}
	});
}


/*---Turn a stats graph on/off when clicking checkbox---*/
function toggleGraph(stats_name){
	var graph_id = document.getElementById('graphs');
	//Populate graph when checkbox is checked
	if($('#do_' + stats_name).is(":checked")){
		graph_id.innerHTML +=  '<div id="' + stats_name + '" class="graph"></div>'
		graphStat(stats_name);
	}
	//Remove graph when checkbox is unchecked
	else{
		document.getElementById(stats_name).remove();
	}
}

/*---Generate the graph of a specific statistic---*/
//TODO: Fix potential scoping issue (set distinct closures)
function graphStat(stats_name){
	signal_name = idToStatName(stats_name);
	//nsample=100;
	nsample = $("#interval").val();
	if(!nsample){
		nsample = -1;
	}
	getTimeValues(signal_name, nsample, function(signal_name, time_axis){
		getMeans(signal_name, nsample, function(signal_name, mean){
			getErrorBars(signal_name, nsample, function(signal_name, error){
				drawGraph(stats_name, signal_name, time_axis, mean, error);
				console.log("Time 0: " + time_axis[0]);
				console.log(time_axis);
			});

		});

	});
}

////////////////////
// Data Fetching //
///////////////////

/*---Get timestamps as an array---*/
function getTimeValues(signal_name, nsample, callback){
	fetch('http://saruman.ra.intel.com:3000/timeaxis/' + signal_name)
		.then(response => response.json())
		.then(time_axis => {
			if(nsample > 0 && nsample <= time_axis.length){
				sub_array = time_axis.slice(time_axis.length-nsample, time_axis.length);
				callback(signal_name, time_axis.slice(time_axis.length-nsample, 
					time_axis.length));
			}
			else{
				callback(signal_name, time_axis);
			}
		});
}

/*---Get stats means as an array---*/
function getMeans(signal_name, nsample, callback){
	fetch('http://saruman.ra.intel.com:3000/mean/' + signal_name)
		.then(response => response.json())
		.then(mean => {
			if(nsample > 0 && nsample <= mean.length){
				sub_array = mean.slice(mean.length-nsample, mean.length);
				callback(signal_name, sub_array);
			}
			else{
				callback(signal_name, mean);
			}
		});
}

/*---Get error bars on a stat as an array---*/
function getErrorBars(signal_name, nsample, callback){
	fetch('http://saruman.ra.intel.com:3000/error/' + signal_name)
		.then(response => response.json())
		.then(error_vals => {
			if(nsample > 0 && nsample < error_vals.length){
				sub_array = error_vals.slice(error_vals.length-nsample, 
					                     error_vals.length);
				callback(signal_name, sub_array);
			}
			else{
				callback(signal_name, error_vals);
			}
		});
}

////////////////////////
// Data Manipulation //
//////////////////////

/*---Generate points to graph error bars as shaded areas---*/
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

////////////////////////////////
// Helper Functions for HTML //
//////////////////////////////

/*---Change statistic name to make it usable as an id in HTML---*/
function idName(statsname){
	var id_name = statsname.replace(" (", "_").replace(")","");
	return id_name;
}

/*---Invert the stats name conversion for lookup in database---*/
function idToStatName(name){
	var stats_name = name.replace("_", " (").concat(")");
	return stats_name
}

//////////////////////
// Graph the thing //
////////////////////


/*---Graph the stat---*/
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
			range: [x[0], x[x.length-1]],
			rangemode: "normal"

		},
		yaxis: {
			title: signal_name,
			exponentformat: "SI"
		}
	};
	Plotly.newPlot(div_id, graph_data, layout);
}

