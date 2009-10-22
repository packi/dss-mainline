Ext.DataView.DragSelector = function(cfg){
	cfg = cfg || {};
	var view, proxy, tracker;
	var rs, bodyRegion, dragRegion = new Ext.lib.Region(0,0,0,0);
	var dragSafe = cfg.dragSafe === true;
	
	this.init = function(dataView){
		view = dataView;
		view.on('render', onRender);
	};
	
	function fillRegions(){
		rs = [];
		view.all.each(function(el){
			rs[rs.length] = el.getRegion();
		});
		bodyRegion = view.el.getRegion();
	}
	function cancelClick(){
		return false;
	}
	
	function onStart(e){
		//view.on('containerclick', cancelClick, view, {single:true});
		
		if(!proxy){
			proxy = view.el.createChild({cls:'x-view-selector'});
		} else {
			proxy.setDisplayed('block');
		}
		fillRegions();
		view.clearSelections();
	}
	
	function onDrag(e){
		var startXY = tracker.startXY;
		var xy = tracker.getXY();
		var x = Math.min(startXY[0], xy[0]);
		var y = Math.min(startXY[1], xy[1]);
		var w = Math.abs(startXY[0] - xy[0]);
		var h = Math.abs(startXY[1] - xy[1]);
		dragRegion.left = x;
		dragRegion.top = y;
		dragRegion.right = x+w;
		dragRegion.bottom = y+h;
		dragRegion.constrainTo(bodyRegion);
		proxy.setRegion(dragRegion);
		for(var i = 0, len = rs.length; i < len; i++){
			var r = rs[i], sel = dragRegion.intersect(r);
			if(sel && !r.selected){
				r.selected = true;
				view.select(i, true);
			} else if(!sel && r.selected) {
				r.selected = false;
				view.deselect(i);
			}
		}
	}
	
	function onEnd(e){
		if (!Ext.isIE) {
			view.un('containerclick', cancelClick, view);
		}
		if(proxy){
			proxy.setDisplayed(false);
		}
	}
	
	function onRender(view){
		tracker = new Ext.dd.DragTracker();//{preventDefault: true});
		tracker.on({
			'dragstart': {
				fn: onStart,
				scope: this
			},
			'drag': {
				fn: onDrag,
				scope: this
			},
			'dragend': {
				fn: onEnd,
				scope:this
			}
		});
		tracker.initEl(view.el);
	}
};
