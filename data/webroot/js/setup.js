Ext.onReady(function(){
	// create a record for a device
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
	
	var deviceFields = [
		 {name: 'name'},
		 {name: 'id'},
		 {name: 'on'},
		 {name: 'circuit'},
		 {name: 'modulator'},
		 {name: 'zone'}
	];
	
	// create the data store
	var deviceStore = new Ext.data.Store({
		fields : deviceFields,
		reader : deviceReader//,
		// data: deviceData
	});
	
	var deviceCols = [
		{id: 'id', header: "id",  width: 50, sortable: true, dataIndex: 'id'},
		{id: 'name', header: "name", width: 50, sortable: true, dataIndex: 'name'},
		{header: "on", width: 50, sortable: true, dataIndex: 'on'},
		{header: "circuit", width: 50, sortable: true, dataIndex: 'circuit'},
		{header: "modulator", width: 50, sortable: true, dataIndex: 'modulator'},
		{header: "zone", width: 50, sortable: true, dataIndex: 'zone'},
	];
	
	var deviceGrid = new Ext.grid.GridPanel({
		store            : deviceStore,
		columns          : deviceCols,
		ddGroup          : "theGroup",
		enableDragDrop   : true,
		stripeRows       : true,
		autoExpandColumn : 'id',
		width            : 500,
		region           : 'center',
		title            : 'Devices'
	});
	
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
	
	var zoneFields = [
		{name: 'name'},
		{id: 'id'}
	];
	
	var zoneStore = new Ext.data.Store({
		fields : zoneFields,
		reader : zoneReader//,
		//data: zoneData
	});
	
	var zoneTemplate = new Ext.XTemplate(
		'<tpl for=".">',
			'<div class="zone-wrap {css}" id="{id}">',
			'<span>{name}</span>',
			'</div>',
		'</tpl>',
		'<div class="x-clear"></div>'
	);
	
	var zoneView = new Ext.DataView({
		store: zoneStore,
		tpl: zoneTemplate,
		autoHeight:true,
		multiSelect: true,
		overClass:'x-view-over',
		itemSelector:'div.zone-wrap',
		emptyText: 'No Rooms to display',
		plugins: [
			new Ext.DataView.DragSelector()
		],
		listeners: {
			selectionchange: {
				fn: function(dv,nodes){
					var l = nodes.length;
					var s = l != 1 ? 's' : '';
					zonePanel.setTitle("Zones (" + l + ' zone' + s + ' selected)');
					filterDevices();
				}
			}
		}
	});
	
	function filterDevices() {
		var selectedZones = zoneView.getSelectedRecords();
		if(selectedZones.length === 0) {
			deviceStore.clearFilter();
			return
		}
		deviceStore.filterBy(function(record) {
			for(var i = 0; i < selectedZones.length; i++) {
				if(record.data.zone == selectedZones[i].data.id) {
					return true;
				}
			}
			return false;
		});
	}
	
	var reloadAction = new Ext.Action({
		text: 'Reload',
		handler: function(){
			loadData();
		},
		iconCls: 'reloadAction'
	});
	
	var createZoneAction = new Ext.Action({
		text: "New Zone",
		handler: function() {
			Ext.Msg.prompt('Name', 'Name:', function(btn, text){
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
											url: '/json/apartment/zone/setName',
											disableCaching: true,
											method: "GET",
											params: { id: i,
																name: text},
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
			});
		},
		iconCls: 'newZoneAction'
	});
	
	var zonePanel = new Ext.Panel({
		id:'images-view',
		//frame:true,
		width:200,
		//autoHeight:true,
		region: "west",
		//layout:'fit',
		title:'Zones',
		items: zoneView,
		tbar: ['->', createZoneAction, reloadAction]
	});
	zoneView.on('selectionchange', function() {
		//filterDevices();
		return;
	});
	
	zoneView.on('render', function() {
		zoneView.dropZone = new Ext.dd.DropZone(zoneView.getEl(), {
			ddGroup          : "theGroup",
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
						params: {		devid: currentDevice.data.id,
												zone:  record.data.id
										},
						success: function(result, request) {
							try {
								var jsonData = Ext.util.JSON.decode(result.responseText);
								if(jsonData.ok) {
									currentDevice.set("zone", record.data.id);
									deviceStore.commitChanges();
									filterDevices();
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
	});
	
	
	//Simple 'border layout' panel to house both grids
	var displayPanel = new Ext.Panel({
		width: 800,
		height: 700,
		layout: 'border',
		renderTo: 'panel',
		items: [
			deviceGrid,
			zonePanel
		]}
	);
	
	var allDevices = [];
	
	function processStructure(structure) {
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
		zoneStore.loadData({zones: zones});
		allDevices = devices;
	}
	
	function processCircuits(circuits) {
		Ext.each(allDevices, function(device) {
			var circuitID = device.circuitID;
			for( var i = 0; i < circuits.length; i++) {
				if(circuits[i].busid == circuitID) {
					device.circuit = circuits[i].name;
					device.modulator = circuits[i].dsid;
				}
			}
		});
		deviceStore.loadData({devices: allDevices});
		if(zoneStore.getCount() > 1) {
			zoneView.select(1);
		} else {
			zoneView.select(0);
		}
	}
	
	function loadStructure() {
		Ext.Ajax.request({
			url: '/json/apartment/getStructure',
			disableCaching: true,
			method: "GET",
			success: function(result, request) {
				try {
					var jsonData = Ext.util.JSON.decode(result.responseText);
					processStructure(jsonData);
				}
				catch (err) {
					Ext.MessageBox.alert('AJAX Error', 'Error processing server response: "' + err + '"');
				}
			},
			failure: function(result, request) {
				Ext.MessageBox.alert('AJAX Error', 'Could not load "' + request.url + '"');
			},
		});
	}
	
	function loadCircuits() {
		Ext.Ajax.request({
			url: '/json/apartment/getCircuits',
			disableCaching: true,
			method: "GET",
			success: function(result, request) {
				try {
					var jsonData = Ext.util.JSON.decode(result.responseText);
					processCircuits(jsonData.result.circuits);
				}
				catch (err) {
					Ext.MessageBox.alert('AJAX Error', 'Error processing server response: "' + err + '"');
				}
			},
			failure: function(result, request) {
				Ext.MessageBox.alert('AJAX Error', 'Could not load "' + request.url + '"');
			},
		});
	}
	
	function loadData() {
		loadStructure();
		loadCircuits();
	}
	
	loadData();
});
