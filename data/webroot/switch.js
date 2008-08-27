var buttons = [];

function Button(switchID, buttonID, parentSwitch) {
  var self = this;
  this.switchID = switchID;
  this.buttonID = buttonID;
  this.parentSwitch = parentSwitch;
  this.timeoutID = 0;
  this.buttonStrID = "switch_" + switchID + "_button_" + buttonID;

  this.sentTouch = false;
  this.gotDown = false;

  this.reportAction = function(kind, doOnComplete) {
    new Ajax.Request(
      '/json/sim/switch/pressed', 
      { method:'get',
	parameters: 
	  { 
	    device: self.switchID,
            buttonnr: self.buttonID,
            kind: kind
	  },
      
	onComplete: function(transport, json) {
          parentSwitch.updateSwitch();
        }
      }
    );
  }

  this.down = function() {
    self.timeoutID = setTimeout(
      function() { 
        self.sendTouch() 
      }, 
      500
    );
    self.gotDown = true;
  }

  this.sendTouch = function() {
    if(self.gotDown == true) {
      //log('touch');
      self.reportAction('touch');
      self.sentTouch = true;
    }
  }

  this.up = function() {
    if(self.gotDown == true) {
      if(self.sentTouch == true) {
        //log('touch end');
        self.reportAction('touchend');
      } else {
        clearTimeout(self.timeoutID);
        //log('click');
        self.reportAction('click');
      }
    }
    self.gotDown = false;
    self.sentTouch = false;
    self.timeoutID = 0;
  }  
}

function Switch(intoID, switchID, groupID) {
  var self = this;
  this.buttons = [];
  this.switchID = switchID;

  this.groupID = groupID;

  this.generateButtons = function(numButtons) {
    for(var iButton = 0; iButton < numButtons; iButton++) {
      self.buttons[iButton] = new Button(switchID, iButton+1, self);
    }
  }

  this.generateButtons(9);
  var buttonIDTemplate = 'switch_' + switchID + '_button_';
  
  function generateTable() {
    function doRow(startIdx, numCol, widths, height) {
      var result = '<tr>'
      for(var iCol = 0; iCol < numCol; iCol++) {
        var btnIdx = startIdx + iCol;
        result += '<td style="background-color: #ffffff" width="' + widths[iCol] + '" height="' + height + 'px" id="' + buttonIDTemplate + btnIdx + '"/>'
      }
      result += '</tr>';
      return result;
    }
    var result = '<table>';
    result += doRow(1, 3, [20, 70, 20], 20);
    result += doRow(4, 3, [20, 70, 20], 70);
    result += doRow(7, 3, [20, 70, 20], 20);
    result += '</table>';
    return result;
  }

  $(intoID).innerHTML = generateTable();

  function wireUp(buttonNr, buttonObj) {
    var elemName = buttonIDTemplate + buttonNr;
    var elem = $(elemName);
    elem.onmouseup = 
     function() {
       buttonObj.up();                
     };
    elem.onmousedown = 
      function() {
        buttonObj.down();
      };
    elem.onmouseout = 
      function() {
        buttonObj.up();
      }
  }
  for(var iBtn = 0; iBtn < 9; iBtn++) {
    wireUp(iBtn+1, self.buttons[iBtn]);
  }

  this.updateSwitch = function() {

    new Ajax.Request(
      '/json/sim/switch/getstate', 
      { method:'get',
	parameters: 
	  { 
	    device: self.switchID
	  },
      
	onComplete: function(transport, json) {
	  var gid = transport.responseJSON.groupid;
	  var newColor = "";
	  if(gid == 0 /* yellow */) {
	    newColor = "#FFFF00";
	  } else if(gid == 1 /* gray */) {
	    newColor = "#AAAAAA";
	  } else if(gid == 2 /* gray */) {
	    newColor = "#0000FF";
	  } else if(gid == 3 /* cyan */) {
	    newColor = "#00FFFF";
	  } else if(gid == 4 /* red */) {
	    newColor = "#FF0000";
	  } else if(gid == 5 /* magenta */) {
	    newColor = "#FF00FF";
	  } else if(gid == 6 /* green */) {
	    newColor = "#00FF00";
	  } else if(gid == 7 /* black */) {
	    newColor = "#000000";
	  } else if(gid = 8 /* white */) {
	    newColor = "#FFFFFF";
	  }
	  $(buttonIDTemplate + '5').style.backgroundColor = newColor;
	}
      }
    );
  }

  self.updateSwitch();
}
 
