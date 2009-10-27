Ext.namespace('dSS', 'dSS.tree');

dSS.tree.PropertyTreeLoader = Ext.extend(Ext.tree.TreeLoader, {
	constructor: function(config) {
		
		Ext.apply(this, { dataUrl: '/json/property/getChildren', requestMethod: 'GET' });
		dSS.data.DeviceStore.superclass.constructor.call(this, arguments);
	},
	requestData: function(node, callback, scope){
		if(this.fireEvent("beforeload", this, node, callback) !== false){
			this.transId = Ext.Ajax.request({
				method:this.requestMethod,
					url: this.dataUrl||this.url,
					success: this.handleResponse,
					failure: this.handleFailure,
					scope: this,
					argument: {callback: callback, node: node, scope: scope},
					params: this.getParams(node)
				});
		}else{
			// if the load is cancelled, make sure we notify
			// the node that we are done
			this.runCallback(callback, scope || node, []);
		}
	},
	processResponse : function(response, node, callback, scope){
		var json = response.responseText;
		try {
			var o = response.responseData || Ext.decode(json);
			o = o.result;
			node.beginUpdate();
			for(var i = 0, len = o.length; i < len; i++){
				var rawNode = o[i];
				rawNode.text = rawNode.name;
				rawNode.leaf = rawNode.type ===  'none' ? false : true;
				rawNode.path = node.attributes.path === '/' ? 
					node.attributes.path + rawNode.name : node.attributes.path + '/' + rawNode.name;
				var n = this.createNode(rawNode);
				if(n){
					node.appendChild(n);
				}
			}
			node.endUpdate();
			this.runCallback(callback, scope || node, [node]);
		}catch(e){
			this.handleFailure(response);
		}
	},
	getParams: function(node) {
		var buf = [];
		buf.push('path', '=', encodeURIComponent(node.attributes.path));
		return buf.join('');
	}
});