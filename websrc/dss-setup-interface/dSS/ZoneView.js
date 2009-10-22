//= require <dSS/data/ZoneStore>

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
//			autoHeight:true,
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