Ext.namespace('dSS', 'dSS.data');

dSS.data.ZoneStore = Ext.extend(Ext.data.Store, {
	constructor: function(config) {
		var zoneRecord = Ext.data.Record.create([
			{name:"name"},
			{name:"id"},
			{name:"primary"}
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
					'<span>',
						'{name}',
						'<tpl if="primary === true">',
							' *',
						'</tpl>',
					'</span>',
				'</div>',
			'</tpl>',
			'<div class="x-clear"></div>'
		);

		var zoneStore = new dSS.data.ZoneStore();

		Ext.apply(this, {
			store: zoneStore,
			tpl: zoneTemplate,
			singleSelect: true,
			multiSelect: false,
			layout: 'fit',
			style: "overflow: auto",
			overClass:'x-view-over',
			itemSelector:'div.zone-wrap',
			emptyText: 'No Rooms to display'
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
								params: {		devid: currentDevice.get('id'),
														zone:  record.get('id')
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
			title:'Rooms',
			layout: 'border',
			items: [{ xtype: 'dsszoneview', ref: 'zoneView', region: 'center'}]
		});

		this.tbar = this.buildTopToolbar();

		dSS.ZonePanel.superclass.initComponent.apply(this, arguments);
	},

	buildTopToolbar: function() {
		return [{
				iconCls: 'newZoneAction',
				handler: this.createNewZone,
				scope: this
			},
			{
				iconCls: 'editZoneAction',
				handler: this.editZone,
				scope: this
			},
			'->',
			{
				iconCls: 'reloadAction',
				handler: this.reload,
				scope: this
			}
		];
	},

	editZone: function() {
		var zoneStore = this.zoneView.getStore();
		if(this.zoneView.getSelectionCount() !== 1) {
			return;
		} else {
			var record = this.zoneView.getSelectedRecords()[0];
			Ext.Msg.prompt('Rename room', 'New name for room:', function(btn, text){
				if(text !== record.get('name')) {
					Ext.Ajax.request({
						url: '/json/zone/setName',
						disableCaching: true,
						method: "GET",
						params: { id: record.get('id'),
											newName: text},
						success: function(result, request) {
							try {
								var jsonData = Ext.util.JSON.decode(result.responseText);
								if(jsonData.ok) {
									record.set('name', text);
									record.commit();
								} else {
									Ext.MessageBox.alert('Error', 'Could not rename room');
								}
							}
							catch (err) {
								Ext.MessageBox.alert('Error', 'Could not rename room');
							}
						},
						failure: function(result, request) {
							Ext.MessageBox.alert('Error', 'Could not rename room');
						},
					});
				}
			}, this, false, record.get('name'));
		}
	},

	createNewZone: function() {
		var zoneStore = this.zoneView.getStore();
		Ext.Msg.prompt('Create new room', 'Name for new room:', function(btn, text){
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
			{name:"zone"},
			{name:"firstSeen"},
			{name:"lastDiscovered"}
		]);

		var deviceReader = new Ext.data.JsonReader(
			{
				root:"devices"
			},
			deviceRecord
		);

		Ext.apply(this, { reader: deviceReader, sortInfo: {
    field: 'firstSeen',
    direction: 'DESC' // or 'DESC' (case sensitive for local sorting)
}});
		dSS.data.DeviceStore.superclass.constructor.call(this, arguments);
	}
});

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
			forceFit         : true,
			title            : 'Devices',
			plugins          : [editor],
			viewConfig: {
        autoFill: true
       }
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
				if(record.get('zone') === selectedZones[i].get('id')) {
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
					device.firstSeen = Date.parseDate(device.firstSeen, "U");
					device.lastDiscovered =  Date.parseDate(device.lastDiscovered, "U");
					devices.push(device);
				});
			}
			zones.push({
				id: zone.id,
				name: zone.name ? zone.name : 'No name',
				primary: zone['firstZoneOnModulator'] !== undefined ? true : false
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
Ext.namespace('dSS', 'dSS.tree');

dSS.tree.PropertyTreeLoader = Ext.extend(Ext.tree.TreeLoader, {
	constructor: function(config) {

		Ext.apply(this, { dataUrl: '/json/property/getChildren', requestMethod: 'GET' });
		dSS.data.DeviceStore.superclass.constructor.call(this, arguments);
	},
	requestData: function(node, callback, scope){
		if(this.fireEvent("beforeload", this, node, callback) !== false){
			this.transId = Ext.Ajax.request({
				method:this.requestMethod,
					url: this.dataUrl||this.url,
					success: this.handleResponse,
					failure: this.handleFailure,
					scope: this,
					argument: {callback: callback, node: node, scope: scope},
					params: this.getParams(node)
				});
		}else{
			this.runCallback(callback, scope || node, []);
		}
	},
	processResponse : function(response, node, callback, scope){
		var json = response.responseText;
		try {
			var o = response.responseData || Ext.decode(json);
			o = o.result;
			node.beginUpdate();
			for(var i = 0, len = o.length; i < len; i++){
				var rawNode = o[i];
				rawNode.text = rawNode.name;
				rawNode.leaf = rawNode.type ===  'none' ? false : true;
				rawNode.path = node.attributes.path === '/' ?
					node.attributes.path + rawNode.name : node.attributes.path + '/' + rawNode.name;
				var n = this.createNode(rawNode);
				if(n){
					node.appendChild(n);
				}
			}
			node.endUpdate();
			this.runCallback(callback, scope || node, [node]);
		}catch(e){
			this.handleFailure(response);
		}
	},
	getParams: function(node) {
		var buf = [];
		buf.push('path', '=', encodeURIComponent(node.attributes.path));
		return buf.join('');
	}
});

Ext.namespace('dSS');

dSS.SystemPropertyTree = Ext.extend(Ext.Panel, {
	initComponent: function() {

		Ext.apply(this, {
			layout: 'border',
			items: [{
					xtype: 'treepanel',
					ref: 'propertytree',
					region: 'center',
					width: 225, // give east and west regions a width
					minSize: 175,
					maxSize: 400,
					autoScroll: true,
					animate: true,
					enableDD: false,
					containerScroll: true,
					border: false,
					loader: new dSS.tree.PropertyTreeLoader(),
					root: new Ext.tree.AsyncTreeNode({
						expanded: false,
						path: '/',
						type: 'none',
						text: 'dSS',
						leaf: false,
					}),
					rootVisible: true,
					listeners: {
						append: this.handleAppend
					}
				}
			]
		});

		dSS.ZoneBrowser.superclass.initComponent.apply(this, arguments);
	},
	handleAppend: function(tree, parent, node, index) {
		if (node.attributes.type === 'none') return;
		var url = "/json/property/";
		switch(node.attributes.type) {
			case 'string':
				url += 'getString';
				break;
			case 'integer':
				url += 'getInteger';
				break;
			case 'boolean':
				url += 'getBoolean';
				break;
			default:
				return;
		}
		Ext.Ajax.request({
			url: url,
			params: { path: node.attributes.path },
			method: 'GET',
			success: function(response, opts) {
				var obj = Ext.decode(response.responseText);
				if(obj.ok === true) {
					node.setText(node.text + " : " + obj.result.value);
				}
			},
			failure: function(response, opts) {
				console.log('server-side failure with status code ' + response.status);
			}
		});
	}
});

Ext.reg('dsssystempropertytree', dSS.SystemPropertyTree);

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
					},{
						title: 'System Properties',
						xtype: 'dsssystempropertytree',
						ref: 'systempropertytree'
					}
				]
			}]
	});
});
