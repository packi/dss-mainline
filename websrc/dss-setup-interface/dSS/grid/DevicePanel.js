//= require <dSS/data/DeviceStore>

Ext.namespace('dSS', 'dSS.grid');

dSS.grid.DevicePanel = Ext.extend(Ext.grid.GridPanel, {
	initComponent: function() {
		
		var deviceCols = [
			{id: 'id', header: "id",  width: 150, sortable: true, dataIndex: 'id'},
			{id: 'name', header: "name", width: 150, sortable: true, dataIndex: 'name', editable: true, editor: new Ext.form.TextField()},
			{header: "on", width: 50, sortable: true, dataIndex: 'on'},
			{header: "circuit", width: 100, sortable: true, dataIndex: 'circuit'},
			{header: "modulator", width: 150, sortable: true, dataIndex: 'modulator'},
			{header: "zone", width: 50, sortable: true, dataIndex: 'zone'},
			{header: "first seen", width: 150, sortable: true, dataIndex: 'firstSeen', xtype: 'datecolumn', format: 'c'},
			{header: "last discovered", width: 150, sortable: true, dataIndex: 'lastDiscovered', xtype: 'datecolumn', format: 'c'}
		];
		
		var deviceStore = new dSS.data.DeviceStore();
		
		Ext.apply(this, {
			store            : deviceStore,
			columns          : deviceCols,
			ddGroup          : "zoneDeviceDD",
			enableDragDrop   : true,
			stripeRows       : true,
			forceFit         : true,
			title            : 'Devices',
			viewConfig: {
				autoFill: true,
				getRowClass: function(record, index) {
					var c = record.get('isPresent');
					if (c === false) {
						return 'nonPresentDevice';
					}
					return '';
				}
			}
		});
		
		dSS.grid.DevicePanel.superclass.initComponent.apply(this, arguments);
		
		// Here you can add functionality that requires the object to
		// exist, like event handling.
		this.on(
			'rowcontextmenu',
			function() {
				//console.log('rowcontextmenu');
			},
			this
		);
	}
});

Ext.reg('dssdevicepanel', dSS.grid.DevicePanel);
