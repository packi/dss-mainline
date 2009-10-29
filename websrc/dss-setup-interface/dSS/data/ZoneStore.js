Ext.namespace('dSS', 'dSS.data');

dSS.data.ZoneStore = Ext.extend(Ext.data.Store, {
	constructor: function(config) {
		// create a record constructor for zone records
		var zoneRecord = Ext.data.Record.create([
			{name:"name"},
			{name:"id"},
			{name:"primary"}
		]);
		
		// a json reader to read the zone data
		var zoneReader = new Ext.data.JsonReader(
			{
				root: "zones"
			},
			zoneRecord
		);
		
		Ext.apply(this, { reader: zoneReader });
		dSS.data.ZoneStore.superclass.constructor.call(this, arguments);
	}
});
