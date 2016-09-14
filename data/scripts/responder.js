/**
 * Stores a property parameter without any type check
 * @param path [in] property path
 * @param value [in] property value
 */
function storeSubParameter(path, value) {
    try
    {
        lInfo.logln('store at |' + path + "| value: |" + value + "|" );
        Property.setProperty('entries/' + path, value);
        Property.setFlag('entries/' + path, 'ARCHIVE', true);
    }
    catch (e)
    {
        lWarn.logln("Exception beim storeSubParameter " + e + " Path " + path  + " value " + value);
    }
}

/**
 * Store a integer property parameter.
 * converts provided value whenever possible to an integer. Discard the the value if this is not possible.
 * @param path [in] property path
 * @param value [in] value to store should be integer
 * @returns {Boolean} returns false if value could not be converted to integer
 */
function checkAndStoreIntParameter(path, value) {
    try
    {
        lInfo.logln('store integer at |' + path + "| value: |" + value + "|" );
        var sucessCode = false;
        var intValue = parseInt("" + value);
        if (!isNaN(intValue)) {
            Property.setProperty('entries/' + path, intValue);
            Property.setFlag('entries/' + path, 'ARCHIVE', true);
            sucessCode = true;
        } else {
            lWarn.logln("Typ-Fehler beim Speichern der Property entries/" + path);
        }
        return sucessCode;
    }
    catch (e)
    {
        lWarn.logln("Exception beim checkAndStoreIntParameter " + e + " Path " + path  + " value " + value);
        return false;
    }
}

/**
 * Store a string property parameter.
 * check weather the provided value is a string. In case it's a number it is converted to string,
 * in any other cases it is discarded
 * @param path [in] property path
 * @param value [in] value to store should be string or at least a number
 * @returns {Boolean} returns false if value could not be converted
 */
function checkAndStoreStringParameter(path, value) {
    try
    {
        lInfo.logln('store string at |' + path + "| value: |" + value + "|" );
        var sucessCode = false;
        var stringValue = (typeof(value) === "number") ? String(value) : value;
        if (typeof(stringValue) === "string") {
            Property.setProperty('entries/' + path, stringValue);
            Property.setFlag('entries/' + path, 'ARCHIVE', true);
            sucessCode = true;
        } else {
            lWarn.logln("Typ-Fehler beim Speichern der Property entries/" + path);
        }
        return sucessCode;
    }
    catch (e)
    {
        lWarn.logln("Exception beim checkAndStoreStringParameter " + e + " Path " + path  + " value " + value);
        return false;
    }
}

/**
 * Store a dsuid as string property parameter.
 * check weather the provided value is a string. In case it's a number it is converted to string,
 * if it is undefined it tries to convert dsid to dsuid, in any other cases it is discarded
 * @param path [in] property path
 * @param dsuid [in] value to store should be string
 * @param dsid [in] dsid value to be converted in dsuid if not available
 * @returns {Boolean} returns false if value could not be converted
 */
function checkAndStoreDSUIDParameter(path, dsuid, dsid) {
    try
    {
        lInfo.logln('store dsuid at |' + path + "| value: |" + dsuid + "|" );
        var successCode = false;
        var stringValue;
        if (typeof(dsuid) === 'string') {
            Property.setProperty('entries/' + path, dsuid);
            Property.setFlag('entries/' + path, 'ARCHIVE', true);
            successCode = true;
        } else {
            if (typeof(dsid) === 'string') {
                stringValue = dSID2dSUID(dsid);
                if (typeof(stringValue) === "string") {
                    Property.setProperty('entries/' + path, stringValue);
                    Property.setFlag('entries/' + path, 'ARCHIVE', true);
                    successCode = true;
                }
                lInfo.logln("store converted dsid");
            } else {
                lWarn.logln('Typ-Fehler beim Speichern der Property entries/' + path)
            }
        }
        return successCode;
    }
    catch (e)
    {
        lWarn.logln("Exception beim checkAndStoreDSUIDParameter " + e + " Path " + path  + " value " + dsuid);
        return false;
    }
}

/**
 * Store a boolean property parameter.
 * convert value if possible. In case In case conversion is not possible
 * the value is discarded.
 * @param path [in] property path
 * @param value [in] value to store should be a boolean
 * @returns {Boolean} returns false if value could not be converted
 */
function checkAndStoreBoolParameter(path, value) {
    try
    {
        lInfo.logln('store boolean at |' + path + "| value: |" + value + "|" );
        var successCode = false;
        var boolValue = false;
        if (typeof(value) === "boolean") {
            Property.setProperty('entries/' + path, value);
            Property.setFlag('entries/' + path, 'ARCHIVE', true);
            successCode = true;
        } else {
            if (typeof(value) === "string") {
                if (value === "true") {
                    boolValue = true;
                }
                Property.setProperty('entries/' + path, boolValue);
                Property.setFlag('entries/' + path, 'ARCHIVE', true);
                successCode = true;
            } else {
                lWarn.logln("Typ-Fehler beim Speichern der Property entries/" + path);
            }
        }
        return successCode;
    }
    catch (e)
    {
        lWarn.logln("Exception beim checkAndStoreBoolParameter " + e + " Path " + path  + " value " + value);
        return false;
    }
}

function dSID2dSUID(dSID) {
    try {
        //copy the first 6 bytes (12 characters to the new dsuid
        var dSUID = dSID.slice(0, 12);
        //insert 4 bytes of 0
        dSUID += '00000000';
        //append the remaining 6 bytes (12 chars) of the dsid
        dSUID += dSID.slice(12, 24);
        //finaly add 1 byte of 0
        dSUID += '00';
        return dSUID;
    } catch (e) {
        lWarn.logln('Exception in store-property-helper::dSID2dSUID ' + e);
        return '';
    }
}

var Utf8 = {
    // public method for url encoding
    encode : function (string) {
        string = string.replace(/\r\n/g,"\n");
        var utftext = "";
        for (var n = 0; n < string.length; n++) {
            var c = string.charCodeAt(n);
            if (c < 128) {
                utftext += String.fromCharCode(c);
            }
            else if((c > 127) && (c < 2048)) {
                utftext += String.fromCharCode((c >> 6) | 192);
                utftext += String.fromCharCode((c & 63) | 128);
            }
            else {
                utftext += String.fromCharCode((c >> 12) | 224);
                utftext += String.fromCharCode(((c >> 6) & 63) | 128);
                utftext += String.fromCharCode((c & 63) | 128);
            }
        }
        return utftext;
    }
};

function storeCondition(idValue,oParameterObject) {
	var succeded = true;
	if (oParameterObject.conditions==null)
		return succeded;
	var sSubpath=idValue + '/conditions/';
	if (oParameterObject.conditions.enabled!=null) {
		if (oParameterObject.conditions.enabled==true)
			storeSubParameter(sSubpath + 'enabled', true);
		else
			storeSubParameter(sSubpath + 'enabled', false);
	}
	if (oParameterObject.conditions.weekdays!=null) {
		var sString="";
		for (var iIndex=0;iIndex<oParameterObject.conditions.weekdays.length;iIndex++) {
			var sWeekday=oParameterObject.conditions.weekdays[iIndex];
			switch (sWeekday)
			{
				case 'SU':
					sString += '0,';
				break;
				case 'MO':
					sString += '1,';
				break;
				case 'TU':
					sString += '2,';
				break;
				case 'WE':
					sString += '3,';
				break;
				case 'TH':
					sString += '4,';
				break;
				case 'FR':
					sString += '5,';
				break;
				case 'SA':
					sString += '6,';
				break;
			}
		}
		if (sString!="") {
			sString=sString.substr(0,sString.length-1);
			storeSubParameter(sSubpath + 'weekdays', sString);
		}
	}
	if (oParameterObject.conditions.systemState!=null) {
		for (var iIndex=0;iIndex<oParameterObject.conditions.systemState.length;iIndex++){
			var sStateName=oParameterObject.conditions.systemState[iIndex].name;
			var sStateValue=oParameterObject.conditions.systemState[iIndex].value;
			if (sStateValue==='false')
				sStateValue=false;
			if (sStateValue==='true')
				sStateValue=true;
			storeSubParameter(sSubpath + 'states/' + sStateName,sStateValue);
		}
	}
	if (oParameterObject.conditions.addonStates!=null) {
		for (var sAddonId in oParameterObject.conditions.addonStates) {
			for (var iIndex=0;iIndex<oParameterObject.conditions.addonStates[sAddonId].length;iIndex++){
				var sStateName=oParameterObject.conditions.addonStates[sAddonId][iIndex].name;
				var sStateValue=oParameterObject.conditions.addonStates[sAddonId][iIndex].value;
				if (sStateValue==='false')
					sStateValue=2;
				if (sStateValue==='true')
					sStateValue=1;
				if (sStateValue===false)
					sStateValue=2;
				if (sStateValue===true)
					sStateValue=1;
				storeSubParameter(sSubpath + 'addon-states/' + sAddonId + '/' + sStateName,sStateValue);
			}
		}
	}
	if (oParameterObject.conditions.zoneState!=null) {
		for (var iIndex=0;iIndex<oParameterObject.conditions.zoneState.length;iIndex++){
			var zoneID=oParameterObject.conditions.zoneState[iIndex].zone;
			var groupID=oParameterObject.conditions.zoneState[iIndex].group;
			var sceneID=oParameterObject.conditions.zoneState[iIndex].scene;

			succeded = checkAndStoreIntParameter(sSubpath + 'zone-states/' + iIndex + '/zone' , zoneID) ? succeded : false;
			succeded = checkAndStoreIntParameter(sSubpath + 'zone-states/' + iIndex + '/group' , groupID) ? succeded : false;
			succeded = checkAndStoreIntParameter(sSubpath + 'zone-states/' + iIndex + '/scene' , sceneID) ? succeded : false;
		}
	}
	if (oParameterObject.conditions.timeframe!=null) {
		for (var iIndex=0;iIndex<oParameterObject.conditions.timeframe.length;iIndex++){
			var from=oParameterObject.conditions.timeframe[iIndex].start;
			var to=oParameterObject.conditions.timeframe[iIndex].end;
			var sFromBase=from.timeBase;
			var sFromOffset=from.offset;
			var sToBase=to.timeBase;
			var sToOffset=to.offset;
			lInfo.logln('from ' + sFromBase + ' off ' + sFromOffset + ' to  ' + sToBase + ' off ' + sToOffset);
			if (sFromBase=='daily')
				if (sToBase=='daily'){
					storeSubParameter(sSubpath + 'time-start' , Math.floor(parseInt("" + sFromOffset,10)/3600) + ':' + Math.floor(parseInt("" + sFromOffset,10)/60)%60 + ':' + Math.floor(parseInt("" + sFromOffset,10)%60));
					storeSubParameter(sSubpath + 'time-end'  , Math.floor(parseInt("" + sToOffset,10)/3600) + ':' + Math.floor(parseInt("" + sToOffset,10)/60)%60 + ':' + Math.floor(parseInt("" + sToOffset,10)%60));
				}
		}
	}
	if (oParameterObject.conditions.date!=null) {
		for (var iIndex=0;iIndex<oParameterObject.conditions.date.length;iIndex++){
			var from=oParameterObject.conditions.date[iIndex].start;
			var to=oParameterObject.conditions.date[iIndex].end;
			var rrule=oParameterObject.conditions.date[iIndex].rrule;

			storeSubParameter(sSubpath + 'date/' + iIndex + '/start' , from );
			storeSubParameter(sSubpath + 'date/' + iIndex + '/end'  , to);
			storeSubParameter(sSubpath + 'date/' + iIndex + '/rrule'  , rrule);

		}
	}
	return succeded;
}

function storeTrigger(idValue,oParameterObject) {
	var succeded = true;
	var iIndex;
    for (iIndex=0;iIndex<oParameterObject.triggers.length;iIndex++)
    {
        var sSubpath=idValue + '/triggers/' + (iIndex + 1) + "/";
        switch(oParameterObject.triggers[iIndex].type) {
            case 'zone-scene':
            	succeded = checkAndStoreStringParameter(sSubpath + "type" , oParameterObject.triggers[iIndex].type) ? succeded : false;
            	succeded = checkAndStoreIntParameter(sSubpath + "zone" , oParameterObject.triggers[iIndex].zone) ? succeded : false;
            	succeded = checkAndStoreIntParameter(sSubpath + "group" , oParameterObject.triggers[iIndex].group) ? succeded : false;
            	succeded = checkAndStoreIntParameter(sSubpath + "scene" , oParameterObject.triggers[iIndex].scene) ? succeded : false;
                if (oParameterObject.triggers[iIndex].forced!=null)
                	succeded = checkAndStoreBoolParameter(sSubpath + "forced" , oParameterObject.triggers[iIndex].forced) ? succeded : false;
                storeSubParameter(sSubpath + "dsuid" , -1);
                break;
            case 'undo-zone-scene':
            	succeded = checkAndStoreStringParameter(sSubpath + "type" , oParameterObject.triggers[iIndex].type) ? succeded : false;
            	succeded = checkAndStoreIntParameter(sSubpath + "zone" , oParameterObject.triggers[iIndex].zone) ? succeded : false;
            	succeded = checkAndStoreIntParameter(sSubpath + "group" , oParameterObject.triggers[iIndex].group) ? succeded : false;
            	succeded = checkAndStoreIntParameter(sSubpath + "scene" , oParameterObject.triggers[iIndex].scene) ? succeded : false;
                storeSubParameter(sSubpath + "dsid" , -1);
                break;
            case 'device-msg':
            	succeded = checkAndStoreStringParameter(sSubpath + "type" , oParameterObject.triggers[iIndex].type) ? succeded : false;
            	//succeded = checkAndStoreStringParameter(sSubpath + "dsid" , oParameterObject.triggers[iIndex].dsid) ? succeded : false;
            	succeded = checkAndStoreDSUIDParameter(sSubpath + "dsuid" , oParameterObject.triggers[iIndex].dsuid, oParameterObject.triggers[iIndex].dsid) ? succeded : false;
            	succeded = checkAndStoreIntParameter(sSubpath + "msg" , oParameterObject.triggers[iIndex].msg) ? succeded : false;
                if (oParameterObject.triggers[iIndex].buttonIndex!=null)
                	succeded = checkAndStoreIntParameter(sSubpath + "buttonIndex" , oParameterObject.triggers[iIndex].buttonIndex) ? succeded : false;
                break;
            case 'device-action':
            	succeded = checkAndStoreStringParameter(sSubpath + "type" , oParameterObject.triggers[iIndex].type) ? succeded : false;
            	succeded = checkAndStoreDSUIDParameter(sSubpath + "dsuid" , oParameterObject.triggers[iIndex].dsuid, oParameterObject.triggers[iIndex].dsid) ? succeded : false;
            	succeded = checkAndStoreIntParameter(sSubpath + "action" , oParameterObject.triggers[iIndex].action) ? succeded : false;
            	break;
            case 'device-binary-input':
            	succeded = checkAndStoreStringParameter(sSubpath + "type" , oParameterObject.triggers[iIndex].type) ? succeded : false;
            	//succeded = checkAndStoreStringParameter(sSubpath + "dsid" , oParameterObject.triggers[iIndex].dsid) ? succeded : false;
            	succeded = checkAndStoreIntParameter(sSubpath + "index" , oParameterObject.triggers[iIndex].index) ? succeded : false;
            	succeded = checkAndStoreStringParameter(sSubpath + "state" , oParameterObject.triggers[iIndex].state) ? succeded : false;
                break;
            case 'device-scene':
            	succeded = checkAndStoreStringParameter(sSubpath + "type" , oParameterObject.triggers[iIndex].type) ? succeded : false;
            	//succeded = checkAndStoreStringParameter(sSubpath + "dsid" , oParameterObject.triggers[iIndex].dsid) ? succeded : false;
            	succeded = checkAndStoreDSUIDParameter(sSubpath + "dsuid" , oParameterObject.triggers[iIndex].dsuid, oParameterObject.triggers[iIndex].dsid) ? succeded : false;
            	succeded = checkAndStoreIntParameter(sSubpath + "scene" , oParameterObject.triggers[iIndex].scene) ? succeded : false;
                break;
            case 'custom-event':
            	succeded = checkAndStoreStringParameter(sSubpath + "type" , oParameterObject.triggers[iIndex].type) ? succeded : false;
            	succeded = checkAndStoreIntParameter(sSubpath + "event" , oParameterObject.triggers[iIndex].event) ? succeded : false;
                break;
            case 'device-sensor':
            	succeded = checkAndStoreStringParameter(sSubpath + "type" , oParameterObject.triggers[iIndex].type) ? succeded : false;
            	//succeded = checkAndStoreStringParameter(sSubpath + "dsid" , oParameterObject.triggers[iIndex].dsid) ? succeded : false;
            	succeded = checkAndStoreDSUIDParameter(sSubpath + "dsuid" , oParameterObject.triggers[iIndex].dsuid, oParameterObject.triggers[iIndex].dsid) ? succeded : false;
            	succeded = checkAndStoreStringParameter(sSubpath + "eventid" , oParameterObject.triggers[iIndex].eventid) ? succeded : false;
                break;
            case 'state-change':
            	succeded = checkAndStoreStringParameter(sSubpath + "type" , oParameterObject.triggers[iIndex].type) ? succeded : false;
            	succeded = checkAndStoreStringParameter(sSubpath + "name" , oParameterObject.triggers[iIndex].name) ? succeded : false;
            	succeded = checkAndStoreStringParameter(sSubpath + "state" , oParameterObject.triggers[iIndex].state) ? succeded : false;
                break;
            case 'addon-state-change':
            	succeded = checkAndStoreStringParameter(sSubpath + "type" , oParameterObject.triggers[iIndex].type) ? succeded : false;
            	succeded = checkAndStoreStringParameter(sSubpath + "addon-id" , oParameterObject.triggers[iIndex].addonId) ? succeded : false;
            	succeded = checkAndStoreStringParameter(sSubpath + "name" , oParameterObject.triggers[iIndex].name) ? succeded : false;
            	succeded = checkAndStoreStringParameter(sSubpath + "state" , oParameterObject.triggers[iIndex].state) ? succeded : false;
                break;
            case 'event':
                succeded = checkAndStoreStringParameter(sSubpath + "type" , oParameterObject.triggers[iIndex].type) ? succeded : false;
                succeded = checkAndStoreStringParameter(sSubpath + "name" , oParameterObject.triggers[iIndex].name) ? succeded : false;
                sSubpath += "parameter/";
                for (var key in oParameterObject.triggers[iIndex].parameter) {
                    succeded = checkAndStoreStringParameter(sSubpath + key , oParameterObject.triggers[iIndex].parameter[key]) ? succeded : false;
                }
                break;
        }
    }
    return succeded;
}

function storeAction(idValue,oParameterObject) {
	var iIndex;
	var succeded = true;
    for (iIndex = 0; iIndex < oParameterObject.actions.length; iIndex++) {
        var oSubobject = oParameterObject.actions[iIndex];
        switch (oSubobject.type) {
	        case 'device-action':
        	success = checkAndStoreDSUIDParameter(idValue + '/actions/' + iIndex + '/dsuid', oSubobject.dsuid, oSubobject.dsid) ? success : false;
	            success = checkAndStoreStringParameter(idValue + '/actions/' + iIndex + '/id', oSubobject.id) ? success : false;
	        break;
		    case 'heating-mode':
		    	succeded = checkAndStoreStringParameter(idValue + '/actions/' + iIndex + '/type', oSubobject.type) ? succeded : false ;
		    	succeded = checkAndStoreIntParameter(idValue + '/actions/' + iIndex + '/zone', oSubobject.zone) ? succeded : false ;
		    	if (oSubobject.reset!=null)
		    		succeded = checkAndStoreBoolParameter(idValue + '/actions/' + iIndex + '/reset', oSubobject.reset) ? succeded : false;
		    	succeded = checkAndStoreIntParameter(idValue + '/actions/' + iIndex + '/mode', oSubobject.mode) ? succeded : false;
		    break;
		    case 'zone-scene':
		    	succeded = checkAndStoreStringParameter(idValue + '/actions/' + iIndex + '/type', oSubobject.type) ? succeded : false ;
		    	succeded = checkAndStoreIntParameter(idValue + '/actions/' + iIndex + '/zone', oSubobject.zone) ? succeded : false ;
		    	succeded = checkAndStoreIntParameter(idValue + '/actions/' + iIndex + '/group', oSubobject.group) ? succeded : false ;
		    	succeded = checkAndStoreIntParameter(idValue + '/actions/' + iIndex + '/scene', oSubobject.scene) ? succeded : false;
		    	if (oSubobject.force!=null)
		    		succeded = checkAndStoreBoolParameter(idValue + '/actions/' + iIndex + '/force', oSubobject.force) ? succeded : false;
		    break;
            case 'undo-zone-scene':
            	succeded = checkAndStoreStringParameter(idValue + '/actions/' + iIndex + '/type', oSubobject.type) ? succeded : false;
            	succeded = checkAndStoreIntParameter(idValue + '/actions/' + iIndex + '/zone', oSubobject.zone) ? succeded : false;
            	succeded = checkAndStoreIntParameter(idValue + '/actions/' + iIndex + '/group', oSubobject.group) ? succeded : false;
            	succeded = checkAndStoreIntParameter(idValue + '/actions/' + iIndex + '/scene', oSubobject.scene) ? succeded : false;
            	if (oSubobject.force!=null)
            		succeded = checkAndStoreBoolParameter(idValue + '/actions/' + iIndex + '/force', oSubobject.force) ? succeded : false;
            break;
            case 'device-scene':
            	succeded = checkAndStoreStringParameter(idValue + '/actions/' + iIndex + '/type', oSubobject.type) ? succeded : false;
            	//succeded = checkAndStoreStringParameter(idValue + '/actions/' + iIndex + '/dsid', oSubobject.dsid) ? succeded : false;
            	succeded = checkAndStoreDSUIDParameter(idValue + '/actions/' + iIndex + '/dsuid' , oSubobject.dsuid, oSubobject.dsid) ? succeded : false;
            	succeded = checkAndStoreIntParameter(idValue + '/actions/' + iIndex + '/scene', oSubobject.scene) ? succeded : false;
            	if (oSubobject.force!=null)
            		succeded = checkAndStoreBoolParameter(idValue + '/actions/' + iIndex + '/force', oSubobject.force) ? succeded : false;
            break;
            case 'device-value':
            	succeded = checkAndStoreStringParameter(idValue + '/actions/' + iIndex + '/type', oSubobject.type) ? succeded : false;
            	succeded = checkAndStoreIntParameter(idValue + '/actions/' + iIndex + '/value', oSubobject.value) ? succeded : false;
            	//succeded = checkAndStoreStringParameter(idValue + '/actions/' + iIndex + '/dsid', oSubobject.dsid) ? succeded : false;
            	succeded = checkAndStoreDSUIDParameter(idValue + '/actions/' + iIndex + '/dsuid' , oSubobject.dsuid, oSubobject.dsid) ? succeded : false;
            break;
            case 'device-blink':
            	succeded = checkAndStoreStringParameter(idValue + '/actions/' + iIndex + '/type', oSubobject.type) ? succeded : false;
            	//succeded = checkAndStoreStringParameter(idValue + '/actions/' + iIndex + '/dsid', oSubobject.dsid) ? succeded : false;
            	succeded = checkAndStoreDSUIDParameter(idValue + '/actions/' + iIndex + '/dsuid' , oSubobject.dsuid, oSubobject.dsid) ? succeded : false;
            break;
            case 'zone-blink':
            	succeded = checkAndStoreStringParameter(idValue + '/actions/' + iIndex + '/type', oSubobject.type) ? succeded : false;
            	succeded = checkAndStoreIntParameter(idValue + '/actions/' + iIndex + '/zone', oSubobject.zone) ? succeded : false;
            	succeded = checkAndStoreIntParameter(idValue + '/actions/' + iIndex + '/group', oSubobject.group) ? succeded : false;
            break;
            case 'custom-event':
            	succeded = checkAndStoreStringParameter(idValue + '/actions/' + iIndex + '/type', oSubobject.type) ? succeded : false;
            	succeded = checkAndStoreStringParameter(idValue + '/actions/' + iIndex + '/event', oSubobject.event) ? succeded : false;
            break;
            case 'url':
            	succeded = checkAndStoreStringParameter(idValue + '/actions/' + iIndex + '/type', oSubobject.type) ? succeded : false;
                oSubobject.url = oSubobject.url.replace(/&amp;/g, '&');
            	succeded = checkAndStoreStringParameter(idValue + '/actions/' + iIndex + '/url', Utf8.encode(unescape(oSubobject.url))) ? succeded : false;
            break;
            case 'change-state':
            	succeded = checkAndStoreStringParameter(idValue + '/actions/' + iIndex + '/type', oSubobject.type) ? succeded : false;
            	if (oSubobject.name!=null)
            		succeded = checkAndStoreStringParameter(idValue + '/actions/' + iIndex + "/statename" , oSubobject.name) ? succeded : false;
            	else
            		succeded = checkAndStoreStringParameter(idValue + '/actions/' + iIndex + "/statename" , oSubobject.statename) ? succeded : false;

               succeded = checkAndStoreStringParameter(idValue + '/actions/' + iIndex + "/state" , oSubobject.state);
            break;
            case 'change-addon-state':
            	succeded = checkAndStoreStringParameter(idValue + '/actions/' + iIndex + '/type', oSubobject.type) ? succeded : false;
            	if (oSubobject.name!=null)
            		succeded = checkAndStoreStringParameter(idValue + '/actions/' + iIndex + "/statename" , oSubobject.name) ? succeded : false;
            	else
            		succeded = checkAndStoreStringParameter(idValue + '/actions/' + iIndex + "/statename" , oSubobject.statename) ? succeded : false;

            	succeded = checkAndStoreStringParameter(idValue + '/actions/' + iIndex + "/addon-id" , oSubobject.addonId) ? succeded : false;
                succeded = checkAndStoreStringParameter(idValue + '/actions/' + iIndex + "/state" , oSubobject.state) ? succeded : false;
            break;
            default:
            	//unexpected type
            	succeded = false;
            break;
        }
        if (oSubobject.delay!=null) {
        	succeded = checkAndStoreIntParameter(idValue + '/actions/' + iIndex + '/delay', oSubobject.delay) ? succeded : false;
        }
        storeSubParameter(idValue + '/actions/' + iIndex + '/category', 'manual');
    }
    return succeded;
}
