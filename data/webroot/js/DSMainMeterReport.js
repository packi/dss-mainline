var DSMainMeterReport = Class.create({
	initialize: function() {
		this.receiverTimer = undefined;
		this.senderTimer = undefined;
		this.connection = false;
		this.sendData = true;
		this.receiveInterval = 5000;
		this.sendInterval = 200;
		this.connenctionStatusID = "#connectionStatusText"
		this.connectionButtonID = "#connectionButton";
		this.clearButtonID = "#clearQueueButton";
		this.lastReceived = 0;
		this.sending = false;
	},
	
	xml2Str: function(xmlNode) {
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
	},


	startSender: function() {
		if(this.connection) {
			var self = this;
			this.senderTimer = setTimeout(function() {
				self.sendValue();
			}, this.sendInterval);
		}
	},

	stopSender: function() {
		clearTimeout(this.senderTimer);
	},

	updateQueueLength: function() {
		var length = jQuery(".xmlFragment").length + (this.sending ? -1 : 0);
		jQuery("#queueLength").text(length >= 0 ? length : 0);
	},

	sendValue: function() {
		var value = jQuery("#queueContainer > div:last");
		var self = this;
		if(value.length > 0) {
			this.sending = true;
			value.fadeTo(300, 0, function() {
					self.updateQueueLength();
					self.sending = false;
					jQuery(this).remove();
					var width = jQuery("#queueContainer").width() - 122;
					jQuery("#queueContainer").width(width);
					self.startSender();
			});
		} else {
			this.startSender();
		}
	},

	value2html: function(value) {
		var xmlString = this.xml2Str(value.xml).replace(/>[\n\t\s]*<(?!\/value)/ig, ">\n\t<")
			.replace(/>[\n\t\s]*<(?=\/value)/ig, ">\n<")
			.replace(/</ig, "&lt;")
			.replace(/>/ig, "&gt;");;
		return ("<div class=\"xmlFragment\" id=" + value.time + "><pre>" + xmlString + "</pre></div>");
	},

	loadNewValues: function(values) {
		var self = this;
		jQuery(values).each(function() {
			jQuery("#queueContainer").append(self.value2html(this));
			var width = jQuery("#queueContainer").width() + 122;
			jQuery("#queueContainer").width(width);
		});
		this.updateQueueLength();
	},

	attribute2Time: function(attribute) {
		var d_date = attribute.slice(0, attribute.indexOf(' ')).split('-');
		var d_time = attribute.slice(attribute.indexOf(' ')+1).split(':');
		return (Date.UTC(d_date[0],d_date[1],d_date[2],d_time[0],d_time[1],d_time[2]) * 1);
	},

	parseXML: function(xml) {
		var data = [];
		var self = this;
		jQuery(xml).find('values > value').each(function() {
			data.push({
				time: self.attribute2Time(this.getAttribute('timestamp')),
				value: jQuery(this).find('> value').text() * 1,
				xml: this
			})
		});
		return data;
	},

	fetchXML: function(dismiss) {
		var currentDate = new Date();
		var self = this;
		jQuery.ajax({
			type: "GET",
			url: "metering/metering_consumption_seconds.xml?date=" + currentDate.getTime(),
			dataType: "xml",
			success: function(xml) {
				var values = self.parseXML(xml);
				var newValues = [];
				jQuery(values).each(function() {
					if(this.time > self.lastReceived) {
						self.lastReceived = this.time;
						newValues.push(this);
					}
				});
				if(dismiss) {
					
				}
				// ignore the last element, because it is likely that the value changes
				newValues.pop();
				if(newValues.length > 0) {
					lastReceived = jQuery(newValues).get(newValues.length - 1).time
					if(!dismiss) {
						self.loadNewValues(newValues);
					}
				}
				setTimeout(function() {
					self.fetchXML(false);
				}, self.receiveInterval);
			}
		});
	},

	toggleConnection: function() {
		if(this.connection == true) {
			connection = false;
			this.jQuery(this.connectionButtonID).val("Connect");
			jQuery(this.connenctionStatusID).text("disconnected");
			jQuery(this.connenctionStatusID).animate({
				color: "red"
			}, 500);
			this.stopSender();
		} else {
			this.connection = true;
			jQuery(this.connectionButtonID).val("Disconnect");
			jQuery(this.connenctionStatusID).text("connected");
			jQuery(this.connenctionStatusID).animate({
				color: "green"
			}, 500);
			this.sendValue();
		}
	},
	
	stop: function() {
		this.connection = false;
		this.stopSender();
	},
	
	restart: function() {
		this.connection = true;
		this.sendValue();
	},
	
	start: function() {
		this.fetchXML(true);
		this.restart();
	}
});