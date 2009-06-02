var resolutions = ["five_minutes", "one_hour", "one_day", "one_week", "one_month", "one_year"];
var dSMIDs = [
				"0000000000000000ffc00011", 
				"0000000000000000ffc00012", 
				"0000000000000000ffc00013", 
				"0000000000000000ffc00014", 
				"0000000000000000ffc00015", 
				"0000000000000000ffc00016"
			];
var roomNames = [
				"Arbeitszimmer",
				"Esszimmer",
				"Gang",
				"Wohnzimmer",
				"Schlafzimmer",
				"Bad"
			];

function registerHandlers() {
	for(var i = 1; i <= 6; i++) {
		(function () {
			var roomNumber = i;
			jQuery("#room" + roomNumber).hover(function() {
				jQuery("#firstFloorHover").attr("src", "views/room_" + roomNumber + ".png").css( {visibility: "visible"});
			}, function() {
				jQuery("#firstFloorHover").css({visibility: "hidden"});
			});
		})();
		
		(function () {
			var roomNumber = i;
			jQuery("#room" + roomNumber).click(function() {
				jQuery("#firstFloorSelection").attr("src", "views/room_" + roomNumber + ".png");
				graph.setDSM(dSMIDs[roomNumber-1]);
				jQuery("#graphSelectionTitle").text(roomNames[roomNumber - 1]);
			});
		})();
	}
	jQuery(resolutions).each(function() {
		jQuery("#" + this).click(function() {
			jQuery("#graphSelector li.selected").removeClass("selected");
			jQuery(this).addClass("selected");
			if(jQuery(this).data("hovering") == true) {
				jQuery(this).data("hovering", false);
			}
			graph.setResolution(jQuery(this).attr("id"));
		});
		var hovering;
		jQuery("#" + this).hover(function() {
			if(jQuery(this).hasClass("selected")) {
				jQuery(this).data("hovering", false);
			} else {
				jQuery(this).addClass("selected");
				jQuery(this).data("hovering", true);
			}
		}, function() {
			if(jQuery(this).data("hovering") == true) {
				jQuery(this).removeClass("selected");
				jQuery(this).data("hovering", false);
			}
		})
	});
	jQuery("#threeD").click(function() {
		jQuery("#viewMode").css({backgroundImage: "url(images/view-mode-3d.png)"});
	});
	jQuery("#map").click(function() {
		jQuery("#viewMode").css({backgroundImage: "url(images/view-mode-map.png)"});
	});
	jQuery("#toggleQueue").data("queueOpen", false).click(function() {
		toggleMainMeterQueue();
	});
	jQuery("#connectionStatusText").data("connected", true).click(function() {
		toggleConnection();
	});
	//var scrollbar = new Control.ScrollBar('scrollbar_content','scrollbar_track');
}

function setConsumptionOverlayForRoom(roomNumber, state) {
	jQuery("#consumptionOverlay" + roomNumber).attr("src", "views/room_" + roomNumber + "_" + state + ".png");
}

function toggleMainMeterQueue() {
	var button = jQuery("#toggleQueue");
	if(button.data("queueOpen")) {
		button.data("queueOpen", false).css({backgroundImage: "url(images/button-open.png)"});
		jQuery("#queue").slideUp(500);
	} else {
		button.data("queueOpen", true).css({backgroundImage: "url(images/button-close.png)"});
		jQuery("#queue").slideDown(500);
	}
}

function toggleConnection() {
	var button = jQuery("#connectionStatusText");
	console.log("toggling connection: " + button);
	if(button.data("connected")) {
		console.log("disconnecting");
		button.data("connected", false).text("disconnected").css({backgroundImage: "url(images/button-disconnected.png)"});
		reporter.stop();
	} else {
	console.log("connecting");
		button.data("connected", true).text("connected").css({backgroundImage: "url(images/button-connected.png)"});
		reporter.restart();
	}
}

function updatePowerConsumptionOverlays() {
	for(var i = 0; i < dSMIDs.length; i++) {
		updatePowerConsumptionOverlay(i);
	}
	setTimeout(function() {updatePowerConsumptionOverlays();}, 5000);
}

function updatePowerConsumptionOverlay(roomNumber) {
	jQuery.ajax({
		type: "GET",
		url: "/json/circuit/getConsumption?id=" + dSMIDs[roomNumber],
		dataType: "json",
		success: function(data) {
			if(data.ok) {
				if(data.result.consumption > 70000) {
					setConsumptionOverlayForRoom((roomNumber + 1), "red");
				} else if(data.result.consumption > 30000) {
					setConsumptionOverlayForRoom((roomNumber + 1), "yellow");
				} else {
					setConsumptionOverlayForRoom((roomNumber + 1), "green");
				}
			}
		}
	});
}