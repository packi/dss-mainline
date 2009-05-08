/**
  * JavaScript Metering Report Visualisation (creator Matias E. Fernandez @ futureLAB)
  *	
  * This script uses jQuery ( http://www.jquery.com/ )
  *
  */

jQuery.noConflict();

var receiverTimer, senderTimer;
var connection = false;
var sendData = true;
var receiveInterval = 1000, sendInterval = 1000;
var connenctionStatusID = "#connectionStatus"
var connectionButtonID = "#connectionButton";
var clearButtonID = "#clearQueueButton";
var lastReceived = 0;
var sending = false;

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


function startSender() {
	if(connection) {
		senderTimer = setTimeout(function() {
			sendValue();
		}, 500);
	}
}

function stopSender() {
	clearTimeout(senderTimer);
}

function updateQueueLength() {
	var length = jQuery("#receivedData > li").length + (sending ? -1 : 0);
	jQuery("#queueLength").text(length >= 0 ? length : 0);
}

function sendValue() {
	var value = jQuery("#receivedData > li:first");
	if(value.length > 0) {
// 		value.effect("slide", {direction: "right", mode: "hide" }, 500, function() {
// 			jQuery(this).remove();
// 			updateQueueLength();
// 		 	startSender();
// 		});
		sending = true;
		value.fadeTo(400, 0, function() {
				updateQueueLength();
				sending = false;
				jQuery(this).slideUp(400, function() {
					jQuery(this).remove();
					startSender();
		 	})
		});
	} else {
		startSender();
	}
}

function value2html(value) {
	var xmlString = xml2Str(value.xml).replace(/>[\n\t\s]*<(?!\/value)/ig, ">\n\t<")
		.replace(/>[\n\t\s]*<(?=\/value)/ig, ">\n<")
		.replace(/</ig, "&lt;")
		.replace(/>/ig, "&gt;");;
	return ("<li id=" + value.time + "><pre>" + xmlString + "</pre></li>");
}

function loadNewValues(values) {
	jQuery(values).each(function() {
		jQuery("#receivedData").append(value2html(this));
	});
	updateQueueLength();
}

function attribute2Time(attribute) {
	var d_date = attribute.slice(0, attribute.indexOf(' ')).split('-');
	var d_time = attribute.slice(attribute.indexOf(' ')+1).split(':');
	return (Date.UTC(d_date[0],d_date[1],d_date[2],d_time[0],d_time[1],d_time[2]) * 1);
}

function parseXML(xml) {
	var data = [];
	jQuery(xml).find('values > value').each(function() {
		data.push({
			time: attribute2Time(this.getAttribute('timestamp')),
			value: jQuery(this).find('> value').text() * 1,
			xml: this
		})
	});
	return data;
}

function fetchXML() {
	var currentDate = new Date();
	jQuery.ajax({
		type: "GET",
		url: "metering/metering.xml?date=" + currentDate.getTime(),
		dataType: "xml",
		success: function(xml) {
			var values = parseXML(xml);
			var newValues = [];
			jQuery(values).each(function() {
				if(this.time > lastReceived) {
					lastReceived = this.time;
					newValues.push(this);
				}
			});
			// ignore the last element, because it is likely that the value changes
			newValues.pop();
			loadNewValues(newValues);
			setTimeout(function() {
				fetchXML();
			}, 3000);
		}
	});
}

function toggleConnection() {
	if(connection == true) {
		connection = false;
		jQuery(connectionButtonID).val("Connect");
		jQuery(connenctionStatusID).text("disconnected");
		jQuery(connenctionStatusID).animate({
			color: "red"
		}, 500);
		stopSender();
	} else {
		connection = true;
		jQuery(connectionButtonID).val("Disconnect");
		jQuery(connenctionStatusID).text("connected");
		jQuery(connenctionStatusID).animate({
			color: "green"
		}, 500);
		sendValue();
	}
}

function init() {
	toggleConnection();
	fetchXML();
	jQuery(connectionButtonID).click( function() {
		toggleConnection();
	});
	jQuery(clearButtonID).click( function() {
		jQuery("#receivedData > li").remove();
		updateQueueLength();
	})
}

jQuery(document).ready(function() {
	init();
});

