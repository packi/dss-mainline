//= require <dSS/tree/PropertyTreeLoader>

Ext.namespace('dSS', 'dSS.system');

dSS.system.PropertyTree = Ext.extend(Ext.Panel, {
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
						leaf: false
					}),
					rootVisible: true,
					listeners: { append: this.handleAppend }
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

Ext.reg('dsssystempropertytree', dSS.system.PropertyTree);