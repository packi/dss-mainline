//= require <dSS/data/DeviceStore>

Ext.namespace('dSS', 'dSS.grid');

dSS.grid.DevicePanel = Ext.extend(Ext.grid.GridPanel, {
	initComponent: function() {
		
		var deviceCols = [
			{id: 'id', header: "id",  width: 50, sortable: true, dataIndex: 'id'},
			{id: 'name', header: "name", width: 50, sortable: true, dataIndex: 'name', editable: true, editor: new Ext.form.TextField()},
			{header: "on", width: 50, sortable: true, dataIndex: 'on'},
			{header: "circuit", width: 50, sortable: true, dataIndex: 'circuit'},
			{header: "modulator", width: 50, sortable: true, dataIndex: 'modulator'},
			{header: "zone", width: 50, sortable: true, dataIndex: 'zone'},
		];
		
		var editor = new Ext.ux.grid.RowEditor({
			saveText: 'Update'
		});
		
		editor.on('afteredit', function() {
			//console.log('a device has been edited');
			deviceStore.commitChanges();
			filterDevices();
		});
		
		var deviceStore = new dSS.data.DeviceStore();
		
		Ext.apply(this, {
			store            : deviceStore,
			columns          : deviceCols,
			ddGroup          : "zoneDeviceDD",
			enableDragDrop   : true,
			stripeRows       : true,
			autoExpandColumn : 'id',
			title            : 'Devices',
			plugins          : [editor]
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
