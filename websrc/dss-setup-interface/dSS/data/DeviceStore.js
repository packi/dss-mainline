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
			{name:"zone"},
			{name:"isPresent"},
			{name:"firstSeen"},
			{name:"lastDiscovered"}
		]);
		
		// a json reader to read the device data
		var deviceReader = new Ext.data.JsonReader(
			{
				root:"devices"
			},
			deviceRecord
		);
		
		Ext.apply(
			this,
			{
				reader: deviceReader,
				sortInfo: {
					field: 'firstSeen',
					direction: 'DESC' // or 'DESC' (case sensitive for local sorting)
				}
			}
		);
		dSS.data.DeviceStore.superclass.constructor.call(this, arguments);
	}
});
