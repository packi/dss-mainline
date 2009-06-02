var DSButton = Class.create({
	initialize: function(aButtonNumber, aSwitch) {
		this.buttonNumber = aButtonNumber;
		this.parentSwitch = aSwitch;
		this.id = "switch_" + this.parentSwitch.ID + "_button_" + this.buttonNumber;

		var self = this;
		var button = jQuery("#" + this.id);
		button.mousedown(function() {
			self.parentSwitch.mouseDown(self.buttonNumber);
		}).mouseout(function() {
			self.parentSwitch.mouseUp(self.buttonNumber);
		}).mouseup(function() {
			self.parentSwitch.mouseOut(self.buttonNumber);
		});
	},
	
	setColor: function(color) {
		jQuery("#" + this.id).css({ backgroundColor: color });
	}
});

var DSSwitch = Class.create({
	initialize: function(targetID, ID, zoneID, groupID) {
		this.buttons = [];
		this.ID = ID;
		this.groupID = 1;
		this.zoneID = zoneID;
		this.timer = undefined;
		this.touch = false;
		jQuery('#switch_' + this.ID).append(this.createTable());
		this.generateButtons(9);
	},
	
	generateButtons: function(numButtons) {
		for(var iButton = 0; iButton < numButtons; iButton++) {
			this.buttons[iButton] = new DSButton(iButton+1, this);
		}
	},
	
	mouseDown: function(buttonNumber) {
		console.log(buttonNumber + " went down");
		var self = this;
		this.timer = setTimeout(function() {
			self.sendTouch(buttonNumber);
		}, 400);
	},
	
	mouseUp: function(buttonNumber) {
		console.log(buttonNumber + " went up");
		if(this.timer) {
			console.log("timer is set");
			clearTimeout(this.timer);
			if(this.touch == true) {
				console.log("touch is true");
				this.touch = false;
				this.sendTouchEnd(buttonNumber);
			} else {
				this.sendKlick(buttonNumber);
			}
		}
	},
	
	mouseOut: function(buttonNumber) {
		this.mouseUp(buttonNumber);
	},
	
	sendKlick: function(buttonNumber) {
		console.log("klick: " + buttonNumber);
	},
	
	sendTouch: function(buttonNumber) {
		console.log("touch: " + buttonNumber);
		this.touch = true;
		var self = this;
		var request = new Ajax.Request(
			'/json/sim/switch/pressed',
			{
				method:'get',
				parameters: {
					buttonnr: buttonNumber,
					zoneID: this.zoneID,
					groupID: this.groupID
				}
			}
		);
		this.timer = setTimeout(function() {
			self.sendTouch(buttonNumber);
		}, 200);
	},
	
	sendTouchEnd: function(buttonNumber) {
		console.log("touch end: " + buttonNumber);
	},
	
	setPreviousGroup: function() {
		var prevGroupID = this.groupID - 1;
		this.setGroup( (prevGroupID < 1) ? 9 : prevGroupID );
	},
	
	setNextGroup: function() {
		var nextGroupID = this.groupID + 1;
		this.setGroup( (nextGroupID > 9) ? 1 : nextGroupID );
	},
	
	setGroup: function(aGroupID) {
		this.groupID = aGroupID;
		
		var newColor;
		switch(this.groupID) {
			case 1:
				newColor = "#FFFF00"; /* yellow */
				break;
			case 2:
				newColor = "#AAAAAA"; /* gray */
				break;
			case 3:
				newColor = "#0000FF"; /* blue */
				break;
			case 4:
				newColor = "#00FFFF"; /* cyan */
				break;
			case 5:
				newColor = "#FF0000"; /* red */
				break;
			case 6:
				newColor = "#FF00FF"; /* magenta */
				break;
			case 7:
				newColor = "#00FF00"; /* green */
				break;
			case 8:
				newColor = "#000000"; /* black */
				break;
			case 9:
				newColor = "#FFFFFF"; /* white */
				break;
			default:
				newColor = "#ff6600"; // orange is for unknown / broadcast groups
		}
		this.buttons[4].setColor(newColor);
	},
	
	reportTouch: function(buttonID) {
		var request = new Ajax.Request(
			'/json/sim/switch/pressed',
			{ 
				method:'get',
				parameters: {
					buttonnr: buttonID,
					zoneID: this.zoneID,
					groupID: this.groupID
				}
			}
		);
		if(buttonID == 1) {
			self.setGroup(1); // light
		} else if(buttonID == 3) {
			self.setGroup(3); // climate
		} else if(buttonID == 7) {
			var nextGroup = (self.groupID - 1);
			if(nextGroup === 0) {
				nextGroup = 9;
			}
			self.setGroup(nextGroup);
		} else if(buttonID == 9) {
			var nextGroup = (self.groupID + 1);
			if(nextGroup == 10) {
				nextGroup = 1;
			}
			self.setGroup(nextGroup);
		}
	},
	
	createTable: function() {
		console.log("creating table");
		var self = this;
		function createTableRow(startIdx, numCol, widths, height) {
			var result = '<tr>';
			for(var iCol = 0; iCol < numCol; iCol++) {
				var btnIdx = startIdx + iCol;
				result += "<td id=\"switch_" + self.ID + "_button_" + (startIdx + iCol) + "\" class=\"switchButton switchButton" + (startIdx + iCol) + "\"\/>";
			}
			result += '</tr>';
			return result;
		};

		var result = '<table>';
		result += createTableRow(1, 3, [20, 70, 20], 20);
		result += createTableRow(4, 3, [20, 70, 20], 70);
		result += createTableRow(7, 3, [20, 70, 20], 20);
		result += '</table>';
		return result;
	}
});


