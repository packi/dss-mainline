//= require <dSS/ZoneView>

Ext.namespace('dSS');

dSS.ZonePanel = Ext.extend(Ext.Panel, {
	initComponent: function() {
		Ext.apply(this, {
			title:'Rooms',
			layout: 'border',
			items: [{ xtype: 'dsszoneview', ref: 'zoneView', region: 'center'}]
			//tbar: ['->', createZoneAction, reloadAction]
		});
		
		this.tbar = this.buildTopToolbar();
		
		dSS.ZonePanel.superclass.initComponent.apply(this, arguments);
	},
	
	buildTopToolbar: function() {
		return [{
				//text: 'New Zone',
				iconCls: 'newZoneAction',
				handler: this.createNewZone,
				scope: this
			},
			{
				//text: 'Edit Room
				iconCls: 'editZoneAction',
				handler: this.editZone,
				scope: this
			},
			'->',
			{
				//text: 'Reload',
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
						}
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
													zoneStore.loadData({zones: [{id: i, name: text}]}, true);
												} else {
													Ext.MessageBox.alert('Error', 'Could not create Zone: ' + json.message);
												}
											}
											catch (err) {
												Ext.MessageBox.alert('Error', 'Could not create Zone: ' + err);
											}
										},
										failure: function(result, request) {
											Ext.MessageBox.alert('Error', 'Could not create Zone');
										}
									});
									
									
									}
								}
								catch (err) {
									Ext.MessageBox.alert('Error', 'Could not create Zone');
								}
							},
							failure: function(result, request) {
								Ext.MessageBox.alert('Error', 'Could not create Zone');
							}
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
