Ext.namespace('dSS', 'dSS.data');

dSS.data.ZoneStore = Ext.extend(Ext.data.Store, {
	constructor: function(config) {
		var zoneRecord = Ext.data.Record.create([
			{name:"name"},
			{name:"id"}
		]);

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

Ext.namespace('dSS');

dSS.ZoneView = Ext.extend(Ext.DataView, {
	initComponent: function() {

		var zoneTemplate = new Ext.XTemplate(
			'<tpl for=".">',
				'<div class="zone-wrap {css}" id="zone-{id}">',
					'<span>{name}</span>',
				'</div>',
			'</tpl>',
			'<div class="x-clear"></div>'
		);

		var zoneStore = new dSS.data.ZoneStore();

		Ext.apply(this, {
			store: zoneStore,
			tpl: zoneTemplate,
			multiSelect: true,
			layout: 'fit',
			style: "overflow: auto",
			overClass:'x-view-over',
			itemSelector:'div.zone-wrap',
			emptyText: 'No Rooms to display',
			plugins: [
				new Ext.DataView.DragSelector()
			]
		});

		dSS.ZoneView.superclass.initComponent.apply(this, arguments);
		this.on(
			'selectionchange',
			function(dv,nodes) {
				var l = nodes.length;
				var s = l != 1 ? 's' : '';
				this.findParentByType('dsszonepanel').setTitle("Zones (" + l + ' zone' + s + ' selected)');
				this.findParentByType('dsszonebrowser').filterDevices();
			},
			this
		);

		this.on(
			'render',
			function() {
				var zoneView = this;
				this.dropZone = new Ext.dd.DropZone(zoneView.getEl(), {
					ddGroup          : "zoneDeviceDD",
					getTargetFromEvent: function(e) {
						return e.getTarget(zoneView.itemSelector);
					},
					onNodeEnter : function(target, dd, e, data){
						Ext.fly(target).addClass('my-row-highlight-class');
					},
					onNodeOut : function(target, dd, e, data){
						Ext.fly(target).removeClass('my-row-highlight-class');
					},
					onNodeOver : function(target, dd, e, data){
						return Ext.dd.DropZone.prototype.dropAllowed;
					},
					onNodeDrop : function(target, dd, e, data){
						var record = zoneView.getRecord(target);
						Ext.each(data.selections, function(device) {
							var currentDevice = device;
							Ext.Ajax.request({
								url: '/json/structure/zoneAddDevice',
								disableCaching: true,
								method: "GET",
								scope: zoneView,
								params: {		devid: currentDevice.data.id,
														zone:  record.data.id
												},
								success: function(result, request) {
									try {
										var jsonData = Ext.util.JSON.decode(result.responseText);
										if(jsonData.ok) {
											currentDevice.set("zone", record.data.id);
											this.getStore().commitChanges();
											this.findParentByType('dsszonebrowser').filterDevices();
										}
									}
									catch (err) {
										Ext.MessageBox.alert('Error', 'Could not move device "' + device.data.dsid + '"');
									}
								},
								failure: function(result, request) {
									Ext.MessageBox.alert('Error', 'Could not move device "' + device.data.dsid + '"');
								},
							});
						});
						return true;
					}
				});
			},
			this
		);
	}
});

Ext.reg('dsszoneview', dSS.ZoneView);

Ext.namespace('dSS');

dSS.ZonePanel = Ext.extend(Ext.Panel, {
	initComponent: function() {
		Ext.apply(this, {
			title:'Zones',
			layout: 'border',
			items: [{ xtype: 'dsszoneview', ref: 'zoneView', region: 'center'}]
		});

		this.tbar = this.buildTopToolbar();

		dSS.ZonePanel.superclass.initComponent.apply(this, arguments);
	},

	buildTopToolbar: function() {
		return ['->',
			{
				text: 'New Zone',
				iconCls: 'newZoneAction',
				handler: this.createNewZone,
				scope: this
			},
			{
				text: 'Reload',
				iconCls: 'reloadAction',
				handler: this.reload,
				scope: this
			}
		];
	},

	createNewZone: function() {
		var zoneStore = this.zoneView.getStore();
		Ext.Msg.prompt('Create new zone', 'Name for new Zone:', function(btn, text){
			if (btn == 'ok'){
				for(var i = 0; i <= zoneStore.data.length; i++) {
					if(zoneStore.findExact('id', i) === -1) {

						Ext.Ajax.request({
							url: '/json/structure/addZone',
							disableCaching: true,
							method: "GET",
							params: { zoneID: i },
							success: function(result, request) {
								try {
									var jsonData = Ext.util.JSON.decode(result.responseText);
									if(jsonData.ok) {



									Ext.Ajax.request({
										url: '/json/zone/setName',
										disableCaching: true,
										method: "GET",
										params: { id: i,
															newName: text},
										success: function(result, request) {
											try {
												var jsonData = Ext.util.JSON.decode(result.responseText);
												if(jsonData.ok) {


													var newZone = new zoneStore.recordType({id: i, name: text}, i);
													zoneStore.insert(i, newZone);



												}
											}
											catch (err) {
												Ext.MessageBox.alert('Error', 'Could not create Zone');
											}
										},
										failure: function(result, request) {
											Ext.MessageBox.alert('Error', 'Could not create Zone');
										},
									});


									}
								}
								catch (err) {
									Ext.MessageBox.alert('Error', 'Could not create Zone');
								}
							},
							failure: function(result, request) {
								Ext.MessageBox.alert('Error', 'Could not create Zone');
							},
						});
						return;
					}
				}
			}
		})
	},
	reload: function() {
		this.ownerCt.loadData();
	}
});

Ext.reg('dsszonepanel', dSS.ZonePanel);
Ext.namespace('dSS', 'dSS.data');

dSS.data.DeviceStore = Ext.extend(Ext.data.Store, {
	constructor: function(config) {
		var deviceRecord = Ext.data.Record.create([
			{name:"name"},
			{name:"id"},
			{name:"on"},
			{name:"circuit"},
			{name:"modulator"},
			{name:"zone"}
		]);

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

		this.on(
			'rowcontextmenu',
			function() {
			},
			this
		);
	}
});

Ext.reg('dssdevicepanel', dSS.grid.DevicePanel);

Ext.namespace('dSS');

dSS.ZoneBrowser = Ext.extend(Ext.Panel, {
	initComponent: function() {

		Ext.apply(this, {
			layout: 'border',
			items: [{
					xtype: 'dsszonepanel',
					ref: 'zonePanel',
					region: 'west',
					width: 225, // give east and west regions a width
					minSize: 175,
					maxSize: 400,
					split: true
				},{
					xtype: 'dssdevicepanel',
					ref: 'devicePanel',
					minSize: 400,
					region: 'center'
				}
			]
		});

		dSS.ZoneBrowser.superclass.initComponent.apply(this, arguments);
		this.on(
			'afterrender',
			this.loadData,
			this
		);

	},
	allDevices: undefined,
	filterDevices: function() {
		var selectedZones = this.zonePanel.zoneView.getSelectedRecords();
		if(selectedZones.length === 0) {
			this.devicePanel.getStore().clearFilter();
			return
		}
		this.devicePanel.getStore().filterBy(function(record) {
			for(var i = 0; i < selectedZones.length; i++) {
				if(record.data.zone == selectedZones[i].data.id) {
					return true;
				}
			}
			return false;
		});
	},
	processStructure: function(structure) {
		var devices = [], zones = [];
		Ext.each(structure.apartment.zones, function(zone) {
			if(zone.id === 0) { // Skip zone 0
				Ext.each(zone.devices, function(device) {
					devices.push(device);
				});
			}
			zones.push({
				id: zone.id,
				name: zone.id === 0 ? 'Uncategorized' : zone.name
			});
			Ext.each(zone.devices, function(device) {
				for(var i = 0; i < devices.length; i++) {
					if(devices[i].id == device.id) {
						devices[i].zone = zone.id;
					}
				}
			});
		});
		this.zonePanel.zoneView.getStore().loadData({zones: zones});
		this.allDevices = devices;
	},
	processCircuits: function(circuits) {
		Ext.each(this.allDevices, function(device) {
			var circuitID = device.circuitID;
			for( var i = 0; i < circuits.length; i++) {
				if(circuits[i].busid == circuitID) {
					device.circuit = circuits[i].name;
					device.modulator = circuits[i].dsid;
				}
			}
		});
		var deviceStore = this.devicePanel.getStore();
		deviceStore.loadData({devices: this.allDevices});
		if(this.zonePanel.zoneView.getStore().getCount() > 1) {
			this.zonePanel.zoneView.select(1);
		} else {
			this.zonePanel.zoneView.select(0);
		}
	},
	loadStructure: function() {
		Ext.Ajax.request({
			url: '/json/apartment/getStructure',
			disableCaching: true,
			method: "GET",
			scope: this,
			success: function(result, request) {
				try {
					var jsonData = Ext.util.JSON.decode(result.responseText);
					this.processStructure(jsonData);
				}
				catch (err) {
					Ext.MessageBox.alert('AJAX Error', 'Error loading apartment structure: "' + err + '"');
				}
			},
			failure: function(result, request) {
				Ext.MessageBox.alert('AJAX Error', 'Could not load "' + request.url + '"');
			},
		});
	},
	loadCircuits: function() {
		Ext.Ajax.request({
			url: '/json/apartment/getCircuits',
			disableCaching: true,
			method: "GET",
			scope: this,
			success: function(result, request) {
				try {
					var jsonData = Ext.util.JSON.decode(result.responseText);
					this.processCircuits(jsonData.result.circuits);
				}
				catch (err) {
					Ext.MessageBox.alert('AJAX Error', 'Could not load circuits: "' + err + '"');
				}
			},
			failure: function(result, request) {
				Ext.MessageBox.alert('AJAX Error', 'Could not load "' + request.url + '"');
			},
		});
	},
	loadData: function() {
		this.loadStructure();
		this.loadCircuits();
	}

});

Ext.reg('dsszonebrowser', dSS.ZoneBrowser);

Ext.onReady(function(){
	Ext.get('start').remove();
	var viewport = new Ext.Viewport({
		layout: 'border',
		items: [
			{
				xtype: 'box',
				region: 'north',
				height: 32, // give north and south regions a height
				autoEl: {
					tag: 'div',
					html:'<h1>digitalSTROM Setup</h1>'
				}
			}, {
				region: 'center',
				xtype: 'tabpanel',
				activeItem: 0,
				items: [
					{
						title: 'Zones',
						xtype: 'dsszonebrowser',
						ref: 'zonebrowser'
					}
				]
			}]
	});
});
