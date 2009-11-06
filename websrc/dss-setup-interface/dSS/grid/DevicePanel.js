//= require <dSS/data/DeviceStore>

Ext.namespace('dSS', 'dSS.grid');

dSS.grid.DevicePanel = Ext.extend(Ext.grid.GridPanel, {
	initComponent: function() {
		
		var stateRenderer = function(value, metaData, record, rowIndex, colIndex, store) {
			if(record.get('on')) {
				metaData.css = 'isOn';
			} else {
				metaData.css = 'isOff';
			}
			return String.format('<img class="padding-img" src="{0}"/>',Ext.BLANK_IMAGE_URL);
		};
		
		var deviceCols = [
			{header: "on", width: 14, resizable: false, sortable: true, dataIndex: 'on', renderer: stateRenderer},
			{id: 'id', header: "id",  width: 150, sortable: true, dataIndex: 'id'},
			{id: 'name', header: "name", width: 150, sortable: true, dataIndex: 'name', editable: true, editor: new Ext.form.TextField()},
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
			function(grid, rowIndex, event) {
				event.preventDefault();
				event.stopEvent();
				if(!this.contextMenu) {
					this.contextMenu = new Ext.menu.Menu({});
				} else {
					this.contextMenu.removeAll();
				}
				
				var menuItem = new Ext.menu.Item({
					text: 'Edit Device',
					icon: '/images/page_white_edit.png',
					handler: this.editDevice.createDelegate(this, rowIndex, true)
				});
				this.contextMenu.add(menuItem)
				var xy = event.getXY();
				this.contextMenu.showAt(xy);
			},
			this
		);
		
		this.on(
			'cellclick',
			function (grid, rowIndex, columnIndex, event) {
				if(columnIndex === 0) {
					var record = this.getStore().getAt(rowIndex);
					Ext.Ajax.request({
						url: record.get('on') ? '/json/device/turnOff' : '/json/device/turnOn',
						disableCaching: true,
						method: "GET",
						params: { dsid: record.get('id') },
						success: function(result, request) {
							try {
								var jsonData = Ext.util.JSON.decode(result.responseText);
								if(jsonData.ok) {
									record.set('on', record.get('on') ? false : true );
									record.commit();
								}
							} catch(err) {
								Ext.MessageBox.alert('Error', err);
							}
						}
					});
				}
			},
			this
		);
	},
	editDevice: function(item, event, rowIndex) {
		var record = this.getStore().getAt(rowIndex);
		Ext.Msg.prompt('Edit device', 'Name:', function(btn, text){
			if(text !== record.get('name')) {
				Ext.Ajax.request({
					url: '/json/device/setName',
					disableCaching: true,
					method: "GET",
					params: { dsid: record.get('id'),
										newName: text},
					success: function(result, request) {
						try {
							var jsonData = Ext.util.JSON.decode(result.responseText);
							if(jsonData.ok) {
								record.set('name', text);
								record.commit();
							} else {
								Ext.MessageBox.alert('Error', 'Could not rename device');
							}
						}
						catch (err) {
							Ext.MessageBox.alert('Error', 'Could not rename device');
						}
					},
					failure: function(result, request) {
						Ext.MessageBox.alert('Error', 'Could not rename device');
					}
				});
			}
		}, this, false, record.get('name'));
	}
});

Ext.reg('dssdevicepanel', dSS.grid.DevicePanel);
