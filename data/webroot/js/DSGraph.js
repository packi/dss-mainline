/**
  * JavaScript Metering Visualisation (creator Matias E. Fernandez @ futureLAB)
  *
  * This script uses jQuery ( http://www.jquery.com/ )
  * along with flot ( http://code.google.com/p/flot/ )
  *
  */
var resolution = "seconds";
var DSGraph = Class.create({
	initialize: function(adSMID) {
		this.dSMID = adSMID;
		this.timer = undefined;
		this.plot = undefined;
		this.updatePlot = true;
		this.requestID = 1;
		this.currentResolution = "seconds";
		this.reloadInterval = 2000;
		this.baseURL = "metering/";
		this.plotOptions = {
			lines: { show: true, lineWidth: 3 },
			grid: { hoverable: false, clickable: false, borderWidth: 0 },
			yaxis: { 
				labelWidth: 40,
				min: 0,
				max: 100000,
				tickFormatter: this.valueFormatterWatt
			},
			xaxis: {
				mode: "time",
				minTickSize: [1, "minute"],
				tickFormatter: this.timeTickFormatter
			}
		};
		this.resolutions = {
			five_minutes: "seconds",
			one_hour: "10seconds",
			one_day: "5minutely",
			one_week: "halfhourly",
			one_month: "2hourly",
			one_year: "daily"
		};
	},
	
	xml2Str: function(xmlNode) {
		try {
			// Gecko-based browsers, Safari, Opera.
			return (new XMLSerializer()).serializeToString(xmlNode);
		} catch (e) {
			try {
				// Internet Explorer.
				return xmlNode.xml;
			} catch (ee) {
				//Other browsers without XML Serializer
				alert('XMLSerializer not supported');
			}
		}
		return false;
	},
	
	timeTickFormatter: function(val, axis) {
		var d = new Date(val);
		var tick = "";
		switch(resolution) {
			case "seconds":
			case "10seconds":
			case "5minutely":
				tick = tick + ((d.getUTCHours() < 8) ? ( "0" + (d.getUTCHours()+2) ) : (d.getUTCHours()+2) );
				tick = tick + ":";
				tick = tick + ((d.getUTCMinutes() < 10) ? ( "0" + d.getUTCMinutes() ) : d.getUTCMinutes() );
				tick = tick + ":";
				tick = tick + ((d.getUTCSeconds() < 10) ? ( "0" + d.getUTCSeconds() ) : d.getUTCSeconds() );
				break;
			default:
				tick = d.getUTCFullYear();
				tick = tick + "-";
				tick = tick + ((d.getUTCMonth() < 10 ) ? ("0" + d.getUTCMonth() ) : d.getUTCMonth() );
				tick = tick + "-";
				tick = tick + ((d.getUTCDate() < 10) ? ("0" + d.getUTCDate() ) : d.getUTCDate() );
		}
		return tick;
	},
	
	valueFormatterWatt: function(val, axis) {
		// val is in mW
		return (val/1000).toFixed(axis.tickDecimals) + " W";
	},
	
	parseXML: function(xml) {
		var values = jQuery(xml).find('value[timestamp]').get();
		var rawData = [];
		for(var i = 0; i < values.length; i++) {
			var dateAttribute = values[i].getAttribute('timestamp');
			var d_date = dateAttribute.slice(0, dateAttribute.indexOf(' ')).split('-');
			var d_time = dateAttribute.slice(dateAttribute.indexOf(' ')+1).split(':');
			var date = Date.UTC(d_date[0],d_date[1],d_date[2],d_time[0],d_time[1],d_time[2]);
			var value = parseInt( jQuery(values[i]).children("value").text() );
			rawData.push([date, value]);
		}
		rawData.pop();
		return rawData;
	},
	
	reloadPlot: function() {
		var currentDate = new Date();
		var self = this;
		this.requestID += 1;
		var requestID = this.requestID;
		jQuery.ajax({
			type: "GET",
			url: self.baseURL + self.dSMID + "_consumption_" + self.currentResolution + ".xml?date=" + currentDate.getTime(),
			dataType: "xml",
			success: function(xml) {
					var xmlString = self.xml2Str(xml).replace(/</ig, "&lt;").replace(/>/ig, "&gt;");
					jQuery('#xml').html('<pre>' + xmlString + '</pre>');
					
					var rawData = self.parseXML(xml);
					self.redrawPlot(rawData);
					self.updateConsumptionIndicator(rawData);
					if(self.updatePlot && (requestID == self.requestID)) {
						self.timer = setTimeout(function() {
							self.reloadPlot(); }, self.reloadInterval);
					}
				},
			error: function(XMLHttpRequest, textStatus, errorThrown) {
					alert("could not load device data: " + textStatus + "\n" + XMLHttpRequest.responseText);
					self.toggleUpdates();
				}
		});
	},
	
	updateConsumptionIndicator: function(rawData) {
		var lastVal = rawData[rawData.length-1];
		if(lastVal !== undefined) {
			lastVal = lastVal[1];
			if(this.dSMID == "metering") {
				if(lastVal > 70000*6) {
					jQuery("#graphSelectionIcon").css({backgroundImage: "url(images/house-red.png)"});
				} else if(lastVal > 30000*6) {
					jQuery("#graphSelectionIcon").css({backgroundImage: "url(images/house-yellow.png)"});
				} else {
					jQuery("#graphSelectionIcon").css({backgroundImage: "url(images/house-green.png)"});
				}
			} else {
				if(lastVal > 70000) {
					jQuery("#graphSelectionIcon").css({backgroundImage: "url(images/room-red.png)"});
				} else if(lastVal > 30000) {
					jQuery("#graphSelectionIcon").css({backgroundImage: "url(images/room-yellow.png)"});
				} else {
					jQuery("#graphSelectionIcon").css({backgroundImage: "url(images/room-green.png)"});
				}
			}

		}
	},
	
	setResolution: function(aResolution) {
		this.currentResolution = this.resolutions[aResolution];
		resolution = this.currentResolution;
		if(this.updatePlot) {
			clearTimeout(this.timer);
		}
		this.reloadPlot();
	},
	
	setDSM: function(adSMID) {
		this.dSMID = adSMID;
		if(this.dSMID == "metering") {
			this.plotOptions.yaxis.max = 6*100000;
			this.plot = jQuery.plot( jQuery("#curve"), [], this.plotOptions );
		} else {
			this.plotOptions.yaxis.max = 100000;
			this.plot = jQuery.plot( jQuery("#curve"), [], this.plotOptions );
		}
		if(this.updatePlot) {
			clearTimeout(this.timer);
		}
		this.reloadPlot();
	},
	
	toggleUpdates: function() {
		if(this.updatePlot) {
			clearTimeout(this.timer);
			jQuery("#updatePlot").attr("checked", "");
			this.updatePlot = false;
		} else {
			this.updatePlot = true;
			jQuery("#updatePlot").attr("checked", "checked");
			this.reloadPlot();
		}
	},
	
	initPlots: function() {
		this.plot = jQuery.plot( jQuery("#curve"), [], this.plotOptions );
	},
	
	redrawPlot: function(data) {
		this.plot.setData([{data: data, color: "white"}]);
		this.plot.setupGrid();
		this.plot.draw();
	},
	
	start: function() {
		this.initPlots();
		this.reloadPlot();
	},
	
	stop: function() {
		this.updatePlot = false;
		clearTimeout(this.timer);
	}
});