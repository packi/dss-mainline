var height = 300;
var initialized = false;

function start() {
  if(!initialized) {
    $('graph').jg = new jsGraphics($('graph'));
    initialized = true;
  }
  var dummy = new Date();
  fetchXML('metering_xml', 'metering/metering.xml?time=' + dummy);
  initGraph('graph' , 'metering/0000000000000000ffc00010_consumption_seconds.xml?time=' + dummy);
  setTimeout('start()', 5000);
  //initGraph('graph2' , 'xml/graph2.xml?time=' + dummy);
}

function fetchXML(drawWhere, url) {
  new Ajax.Request(url, {
    method: 'get',
    onSuccess: function(transport) {
      var xmlString = transport.responseText;
      var re = new RegExp('<', "g");
      xmlString = xmlString.replace(re, '&lt;');
      re = new RegExp('>', "g");
      xmlString = xmlString.replace(re, '&gt;');
      $(drawWhere).innerHTML = '<pre>' + xmlString + '</pre>';
    }
  });
}

function initGraph(drawWhere, url) {
  new Ajax.Request(url, {
    method: 'get',
    onSuccess: function(transport) {
      var domObj = transport.responseXML;
      var children = domObj.childNodes;
      var valuesElement = children[1].childNodes[3];
      var values = valuesElement.childNodes;


      var xPoints = new Array();
      var yPoints = new Array();
      var xPosition = 0;
      for (var j = 0 ; j < values.length; j++ ) {
        var theValueElement = values[j];
        if(theValueElement.nodeName == 'value') {
          for(var k = 0; k < theValueElement.childNodes.length; k++) {
            if(theValueElement.childNodes[k].nodeName == 'value') {
              xPoints.push(xPosition * 1);
              xPosition++;
              yPoints.push(height - (theValueElement.childNodes[k].textContent * 0.01));
            }
          }
        }
      }
      //console.log('will draw to ' + drawWhere);
//      var jg = //$(drawWhere).jg;
            $(drawWhere).innerHTML = '';

      var jg = new jsGraphics($(drawWhere));
      jg.clear();
      jg.setColor("#ff0000");
      //console.log('will draw : ' + xPoints + ' y : ' + yPoints);
      jg.drawPolyline(xPoints,yPoints);
      jg.paint();
    }
  });
}


