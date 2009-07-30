print("initialize.js, running");
// create subscription for udi demo
var s = subscription("read_temperature", "javascript", { 'filename': 'data/read_temperature.js' } );
s.subscribe();
// raise event read_temperture in 10 seconds
var evt = event("read_temperature", { 'time': '+2'} );
evt.raise();
