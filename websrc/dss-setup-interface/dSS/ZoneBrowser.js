//= require <dSS/ZonePanel>
//= require <dSS/grid/DevicePanel>

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
				name: zone.name ? zone.name : 'No name'
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