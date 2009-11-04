//= require <dSS/ZoneBrowser>
//= require <dSS/SystemPanel>

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
						ref: 'zoneBrowser'
					},{
						title: 'System Properties',
						xtype: 'dsssystempanel',
						ref: 'systemPanel',
						id: 'hurz'
					}
				]
			}]
	});
});
