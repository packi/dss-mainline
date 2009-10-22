Ext.namespace('dSS', 'dSS.data');

dSS.data.DeviceStore = Ext.extend(Ext.data.Store, {
	constructor: function(config) {
		// create a record constructor for device records
		var deviceRecord = Ext.data.Record.create([
			{name:"name"},
			{name:"id"},
			{name:"on"},
			{name:"circuit"},
			{name:"modulator"},
			{name:"zone"}
		]);
		
		// a json reader to read the device data
		var deviceReader = new Ext.data.JsonReader(
			{
				root:"devices"
			},
			deviceRecord
		);
		
		Ext.apply(this, { reader: deviceReader });
		dSS.data.DeviceStore.superclass.constructor.call(this, arguments);
	}
});
