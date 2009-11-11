//= require <dSS/data/ZoneStore>

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
		// Here you can add functionality that requires the object to
		// exist, like event handling.
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
											currentDevice.store.commitChanges();
											this.findParentByType('dsszonebrowser').filterDevices();
										}
									}
									catch (err) {
										Ext.MessageBox.alert('Error', 'Could not move device "' + device.data.dsid + '"');
									}
								},
								failure: function(result, request) {
									Ext.MessageBox.alert('Error', 'Could not move device "' + device.data.dsid + '"');
								}
							});
						});
						return true;
					}
				});
			},
			this
		);
		
		this.on(
			'contextmenu',
			function(view, index, node, event) {
				event.preventDefault();
				event.stopEvent();
				if(!this.contextMenu) {
					this.contextMenu = new Ext.menu.Menu({});
				} else {
					this.contextMenu.removeAll();
				}
				
				var menuItem = new Ext.menu.Item({
					text: 'Delete Room',
					icon: '/images/delete.png',
					handler: this.removeZone.createDelegate(this, index, true)
				});
				this.contextMenu.add(menuItem)
				var xy = event.getXY();
				this.contextMenu.showAt(xy);
			},
			this
		);
	},
	removeZone: function(item, event, index) {
		var record = this.getStore().getAt(index);
		if(record.get('primary') == true) {
			Ext.MessageBox.alert('Error', 'You cannot delete a primary room.');
			return;
		}
		var deviceStore = this.findParentByType('dsszonebrowser').devicePanel.getStore();
		if(deviceStore.query('zone', record.get('id')).getCount() > 0) {
			Ext.MessageBox.alert('Error', 'You cannot delete a non empty room.');
			return;
		}
		
		Ext.Ajax.request({
			url: '/json/structure/removeZone',
			disableCaching: true,
			method: "GET",
			scope: this,
			params: { zoneID: record.get('id') },
			success: function(result, request) {
				try {
					var jsonData = Ext.util.JSON.decode(result.responseText);
					if(jsonData.ok) {
						this.getStore().removeAt(index);
					} else {
						Ext.MessageBox.alert('Error', 'Could not remove room "' + record.get('name') + '"');
					}
				} catch (err) {
						Ext.MessageBox.alert('Error', 'Could not move device "' + record.get('name') + '"');
				}
			},
			failure: function(result, request) {
				Ext.MessageBox.alert('Error', 'Could not remove room "' + record.get('name') + '"');
			}
		})
	}
});

Ext.reg('dsszoneview', dSS.ZoneView);