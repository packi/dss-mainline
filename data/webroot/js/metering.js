/**
  * JavaScript Metering Visualisation (creator Matias E. Fernandez @ futureLAB)
  *	
  * This script uses jQuery ( http://www.jquery.com/ )
  * along with flot ( http://code.google.com/p/flot/ )
  *
  */

jQuery.noConflict();

var devicePlot, devicePlotTimer, meteringPlot, meteringPlotTimer;
var updateDevicePlot = true, updateMeteringPlot = true;
var reloadInterval = 1000;

var plotOptions = {
	lines: { show: true },
	points: { show: false },
	grid: { hoverable: false, clickable: false },
	yaxis: { 
		autoscaleMargin: 0.05, 
		labelWidth: 50,
		labelHeight: 9,
		min: 0
	},
	xaxis: {
		mode: "time",
		minTickSize: [1, "minute"],
		tickFormatter: function (val, axis) {
			var d = new Date(val);
			var tick = "";
			tick = d.getUTCFullYear();
			tick = tick + "-";
			tick = tick + ((d.getUTCMonth() < 10 ) ? ("0" + d.getUTCMonth() ) : d.getUTCMonth() );
			tick = tick + "-";
			tick = tick + ((d.getUTCDate() < 10) ? ("0" + d.getUTCDate() ) : d.getUTCDate() );
			tick = tick + "<br />";
			tick = tick + ((d.getUTCHours() < 10) ? ( "0" + d.getUTCHours() ) : d.getUTCHours() );
			tick = tick + ":";
			tick = tick + ((d.getUTCMinutes() < 10) ? ( "0" + d.getUTCMinutes() ) : d.getUTCMinutes() );
			tick = tick + ":";
			tick = tick + ((d.getUTCSeconds() < 10) ? ( "0" + d.getUTCSeconds() ) : d.getUTCSeconds() );
			return tick;
			
		}
	}
};

function xml2Str(xmlNode) {
   try {
      // Gecko-based browsers, Safari, Opera.
      return (new XMLSerializer()).serializeToString(xmlNode);
  }
  catch (e) {
     try {
        // Internet Explorer.
        return xmlNode.xml;
     }
     catch (e) {  
        //Other browsers without XML Serializer
        alert('XMLSerializer not supported');
     }
   }
   return false;
}

function parseXML(xml) {
	var values = jQuery(xml).find('values').get(0);
	values = jQuery(values).find('> value').get();
	var rawData = new Array();
	var start = values.length - 100;
	start = (start < 0) ? 0 : start;
	for(var i = start; i < values.length; i++) {
		var dateAttribute = values[i].getAttribute('timestamp');
		var d_date = dateAttribute.slice(0, dateAttribute.indexOf(' ')).split('-');
		var d_time = dateAttribute.slice(dateAttribute.indexOf(' ')+1).split(':');
		var date = Date.UTC(d_date[0],d_date[1],d_date[2],d_time[0],d_time[1],d_time[2]) * 1;
		var value = jQuery(values[i]).find('> value').text() * 1;
		rawData.push([date, value]);
	}
	return rawData;
}

function reloadDevicePlot() {
	var currentDate = new Date();
	jQuery.ajax({
		type: "GET",
		url: "metering/0000000000000000ffc00010_consumption_seconds.xml?date=" + currentDate.getTime(),
		dataType: "xml",
		success: function(xml) {
			var rawData = parseXML(xml);
			redrawPlot(devicePlot, rawData);
			if(updateDevicePlot) {
				devicePlotTimer = setTimeout("reloadDevicePlot()", reloadInterval);
			}
		}
	});
}

function toggleDevicePlotUpdates() {
	if(updateDevicePlot) {
		clearTimeout(devicePlotTimer);
		jQuery("#updateDevicePlot").attr('checked', false);
		updateDevicePlot = false;
	} else {
		updateDevicePlot = true;
		jQuery("#updateDevicePlot").attr('checked', true);
		reloadDevicePlot();
	}
}

function reloadMeteringPlot() {
	var currentDate = new Date();
	jQuery.ajax({
		type: "GET",
		url: "metering/metering.xml?date=" + currentDate.getTime(),
		dataType: "xml",
		success: function(xml) {
			var xmlString = xml2Str(xml).replace(/</ig, "&lt;").replace(/>/ig, "&gt;");
			jQuery('#meteringXML').html('<pre>' + xmlString + '</pre>');
			var rawData = parseXML(xml);
			redrawPlot(meteringPlot, rawData);
			if(updateMeteringPlot) {
				meteringPlotTimer = setTimeout("reloadMeteringPlot()", reloadInterval);
			}
		}
	});
}

function toggleMeteringPlotUpdates() {
	if(updateMeteringPlot) {
		clearTimeout(meteringPlotTimer);
		jQuery("#updateMeteringPlot").attr('checked', false);
		updateMeteringPlot = false;
	} else {
		updateMeteringPlot = true;
		jQuery("#updateMeteringPlot").attr('checked', true);
		reloadMeteringPlot();
	}
}

function initPlots() {
	devicePlot = jQuery.plot( jQuery("#devicePlot"), [], plotOptions );
	meteringPlot = jQuery.plot( jQuery("#meteringPlot"), [], plotOptions );
}

function redrawPlot(plot, data) {
	plot.setData([data]);
	plot.setupGrid();
	plot.draw();
}

jQuery(document).ready(function() {
	initPlots();
	
	reloadDevicePlot();
	reloadMeteringPlot();
	
	jQuery("#updateDevicePlot").click(function () {
		toggleDevicePlotUpdates();
	});
	
	jQuery("#updateMeteringPlot").click(function () {
		toggleMeteringPlotUpdates();
	});
});