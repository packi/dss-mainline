//= require <dSS/system/PropertyTree.js>

Ext.namespace('dSS');

dSS.SystemPanel = Ext.extend(Ext.Panel, {
	initComponent: function() {
		
		var tabRecord = Ext.data.Record.create([
			{ name:"title" }
		]);
		
		var tabStore = new Ext.data.Store({}, tabRecord);
		
		
		var contentPanel = {
			ref: 'contentPanel',
			region: 'center',
			layout: 'card',
			activeItem: 0,
			border: false,
			items: [ 
				{
					title: 'Property Tree',
					xtype: 'dsssystempropertytree',
					ref: 'systemPropertyTree'
				}
			]
		};
		
		Ext.apply(this, {
			layout: 'border',
			items: [ {
					xtype: 'listview',
					title: 'System',
					ref: 'listView',
					store: tabStore,
					singleSelect: true,
					region: 'west',
					hideHeaders: true,
					width: 275,
					columns: [{
						header: 'name',
						dataIndex: 'title'
					}]
				},
				contentPanel
			]
		});
		
		dSS.ZoneBrowser.superclass.initComponent.apply(this, arguments);
		this.on(
			'activate',
			function(component) {
				if(this.initializedTabs) {
					return;
				}
				// for each item in contentPanel
				this.contentPanel.items.each(function(item) {
					var tab = new tabRecord({title: item.title});
					tabStore.add([tab]);
				}, this);
				this.listView.select(0, false, true);
				this.initializedTabs = true;
			},
			this
		);
		this.items.get(0).on(
			'selectionchange',
			function(listView, selections) {
				if(selections.length === 1) {
					this.contentPanel.layout.setActiveItem(selections[0].viewIndex);
				}
			},
			this
		);
	}
});

Ext.reg('dsssystempanel', dSS.SystemPanel);