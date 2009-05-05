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
        self.parentSwitch.onClick(self.buttonID);
      }
    }
    self.gotDown = false;
    self.sentTouch = false;
    self.timeoutID = 0;
  }
}

function Switch(intoID, switchID, groupID, zoneID) {
  var self = this;
  this.buttons = [];
  this.switchID = switchID;

  this.groupID = groupID;
  this.zoneID = zoneID;

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

  this.setGroup = function(_groupID) {
    self.groupID = _groupID;
    var newColor = "#ff6600"; // orange is for unknown / broadcast groups
    if(_groupID == 1 /* yellow */) {
      newColor = "#FFFF00";
    } else if(_groupID == 2 /* gray */) {
      newColor = "#AAAAAA";
    } else if(_groupID == 3 /* blue */) {
      newColor = "#0000FF";
    } else if(_groupID == 4 /* cyan */) {
      newColor = "#00FFFF";
    } else if(_groupID == 5 /* red */) {
      newColor = "#FF0000";
    } else if(_groupID == 6 /* magenta */) {
      newColor = "#FF00FF";
    } else if(_groupID == 7 /* green */) {
      newColor = "#00FF00";
    } else if(_groupID == 8 /* black */) {
      newColor = "#000000";
    } else if(_groupID = 9 /* white */) {
      newColor = "#FFFFFF";
    }
    $(buttonIDTemplate + '5').style.backgroundColor = newColor;
  }

  this.onClick = function(_buttonNr) {
    new Ajax.Request(
      '/json/sim/switch/pressed',
      { method:'get',
        parameters:
        {
          buttonnr: _buttonNr,
          zoneID: self.zoneID,
          groupID: self.groupID
        }
      }
    );
    if(_buttonNr == 1) {
      self.setGroup(1); // light
    } else if(_buttonNr == 3) {
      self.setGroup(3); // climate
    } else if(_buttonNr == 7) {
      var nextGroup = self.groupID - 1;
      if(nextGroup == 0) {
        nextGroup = 9;
      }
      self.setGroup(nextGroup);
    } else if(_buttonNr == 9) {
      var nextGroup = self.groupID + 1;
      if(nextGroup == 10) {
        nextGroup = 1;
      }
      self.setGroup(nextGroup);
    }
  }

  self.setGroup(1);
}

