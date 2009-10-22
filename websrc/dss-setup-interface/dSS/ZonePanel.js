//= require <dSS/ZoneView>

Ext.namespace('dSS');

dSS.ZonePanel = Ext.extend(Ext.Panel, {
	initComponent: function() {
		Ext.apply(this, {
			title:'Zones',
			layout: 'border',
			items: [{ xtype: 'dsszoneview', ref: 'zoneView', region: 'center'}]
			//tbar: ['->', createZoneAction, reloadAction]
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
