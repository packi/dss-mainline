/*
 * This file is a modified version of:
 * http://www.arachnoid.com/lutusp/sunrise/solar_computer.js
 *
 * All changes made to the original sources:
 * Copyright (c) 2010 digitalSTROM.org, Zurich, Switzerland
 * License: GPL version 3
 *
 * See copyright and license of the original code below:
 */

/**************************************************************************
 *   Copyright (C) 2006, Paul Lutus                                        *
 *                                                                         *
 *   This program is free software; you can redistribute it and/or modify  *
 *   it under the terms of the GNU General Public License as published by  *
 *   the Free Software Foundation; either version 2 of the License, or     *
 *   (at your option) any later version.                                   *
 *                                                                         *
 *   This program is distributed in the hope that it will be useful,       *
 *   but WITHOUT ANY WARRANTY; without even the implied warranty of        *
 *   MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the         *
 *   GNU General Public License for more details.                          *
 *                                                                         *
 *   You should have received a copy of the GNU General Public License     *
 *   along with this program; if not, write to the                         *
 *   Free Software Foundation, Inc.,                                       *
 *   59 Temple Place - Suite 330, Boston, MA  02111-1307, USA.             *
 ***************************************************************************/

function SunCalc() {
    this.sunRefract = true;

    this.Rad_Deg = 180.0/Math.PI;
    this.Deg_Rad = Math.PI/180.0;

    this.radians = function(v)
    {
        return v * this.Deg_Rad;
    }

    this.degrees = function(v)
    {
        return v * this.Rad_Deg;
    };

    this.convday = 4.16666666666667E-02;
    this.convhour = 6.66666666666667E-02;
    this.convmin = 1.66666666666667E-02;
    this.convsec = 2.77777777777778E-04;
    this.convcircle = 2.77777777777778E-03;
    this.SUNSET = -.833333333333333;
    this.CIVIL = -6.0;
    this.NAUTICAL = -12.0;
    this.ASTRONOMICAL = -18.0;

    // BEGIN class sunRTS
    this.sunRTS = function() {
    };
    this.sunRTS.prototype.jd = undefined;
    this.sunRTS.prototype.lat = undefined;
    this.sunRTS.prototype.lng = undefined;
    this.sunRTS.prototype.gha = undefined;
    this.sunRTS.prototype.rise = undefined;
    this.sunRTS.prototype.transit = undefined;
    this.sunRTS.prototype.set = undefined;
    this.sunRTS.prototype.clone = function(o) {
        this.rise = o.rise;
        this.transit = o.transit;
        this.set = o.set;
        this.jd = o.jd;
    };
    // END class sunRTS

    // BEGIN class datim
    this.datim = function() {
    };
    this.datim.prototype.jd = undefined;
    this.datim.prototype.mo = undefined;
    this.datim.prototype.da = undefined;
    this.datim.prototype.yr = undefined;
    this.datim.prototype.dow = undefined;
    this.datim.prototype.hh = undefined;
    this.datim.prototype.mm = undefined;
    this.datim.prototype.ss = undefined;
    this.datim.prototype.sgn = undefined;
    // END class datim

    this.leadChar = function(v,c,lead)
    {
        s = "" + v;
        while(s.length < lead)
            s = c + s;
        return s;
    };

    // month 1 - 12, day 1-31 year may be 2 digit by 4 is preferred
    this.mdy_jd = function(yr,mo,da,hh,mm,ss)
    {
        var a,b,jd;
        if(yr < 100.0)
            yr += ((yr > 50)?1900.0:2000.0); // try to cope with date problem
        if(mo < 3.0)
        {
            yr -= 1.0;
            mo += 12.0;
        }
        a = da + (mo * 100.0) + (yr * 10000.0);
        if (a < 15821015.0) /* Julian calendar */
        {
            b = 0.0;
        }
        else /* Gregorian */
        {
            a = Math.floor(yr/100.0);
            b = 2 - a + Math.floor(a/4.0);
        }
        jd = Math.floor(365.25*(yr+4716.0)) + Math.floor(30.6001*(mo+1)) + da + b - 1524.5;
        jd += (hh/24.0) + (mm/1440.0) + (ss/86400.0);
        return(jd);
    };

    this.jd_mdy = function(jd)
    {
        var z,f,a,b,c,d,e;
        var r = new datim();
        jd += .5;
        z = Math.floor(jd);
        f = jd-z;
        if(z < 2299161) /* Julian calendar */
        {
            a = z;
        }
        else /* Gregorian */
        {
            b = Math.floor((z - 1867216.25)/36524.25);
            a = z + 1 + b - Math.floor(b/4);
        }
        b = a + 1524;
        c = Math.floor((b - 122.1)/365.25);
        d = Math.floor(365.25*c);
        e = Math.floor((b-d)/30.6001);
        r.mo = (e < 14)?e-1:e-13;
        r.da = b - d - Math.floor(30.6001*e) + f;
        r.yr = (r.mo < 3)?c-4715:c-4716;
        /* now get h,m,s */
        a = r.da;
        r.da = Math.floor(r.da);
        r.hh = 24 * (a-r.da);
        a = r.hh;
        r.hh = Math.floor(r.hh);
        r.mm = (60 * (a - r.hh));
        a = r.mm;
        r.mm = Math.floor(r.mm);
        r.ss = 60 * (a - r.mm);
        r.ss = Math.floor(r.ss);
        return(r);
    };

    this.el_dd = function(sid,lat,decl,ra) // sun azimuth for ra, decl at this loc
    {
        var lha;
        var x;
        lat = radians(lat);
        decl = radians(decl);
        lha = radians(sid - ra);
        x = this.degrees(Math.asin(Math.sin(lat) * Math.sin(decl) + Math.cos(lat) * Math.cos(decl) * Math.cos(lha)));
        /* correct for atmospheric refraction */
        if(sunRefract)
        {
            x = x + (1.92792040346393E-03 + (1.02 / Math.tan(radians(x + (10.3 / (x + 5.11)))))) * convmin;
        }
        return(x);
    };

    /* azimuth 0 to 360 for object. az_ns allows 0 = south, magdec has local magnetic declination */
    this.az_dd = function(sid,lat,decl,ra,magdecl) // sun elevation for ra, decl at this loc
    {
        var lha,x,a,b;
        lat = radians(lat);
        decl = radians(decl);
        lha = radians(sid - ra);
        a = Math.sin(lha);
        b = Math.cos(lha) * Math.sin(lat) - Math.tan(decl) * Math.cos(lat);
        x = this.degrees(Math.atan2(a,b));
        if (x < 0.0)
            x = 360.0 + x;
        x = x + 180.0;
        x = x + magdecl;
        // now force x into 0 - 360 value range
        x = x * this.convcircle;
        x = x - Math.floor(x);
        x = x * 360.0;
        return(x);
    };

    this.calcsun = function(jd,hh,mm,ss,tz) // compute sun's position on this date
    {
        var s = new this.sunRTS();
        var h;
        //var jd;
        var t;
        var lo;
        var m;
        var rm;
        var e;
        var c;
        var tl;
        var v;
        var r;
        var nut;
        var al;
        var obliq;
        var sunra;
        var sundecl;
        jd = jd-(tz/24.0); /* convert to GMT */
        h = ((hh) + (mm/60.0) + (ss/3600.0)) * 15.0;
        t = (jd - 2451545) * 2.7378507871321E-05;
        lo = 280.46645 + (36000.76983 * t) + (0.0003032 * t * t);
        m = 357.5291 + (35999.0503 * t) - (0.0001559 * t * t) - (0.00000048 * t * t * t);
        rm = this.radians(m);
        e = 0.016708617 - (0.000042037 * t) - (0.0000001236 * t * t);
        c = (1.9146 - 0.004817 * t - 0.000014 * t * t) * Math.sin(rm);
        c = c + (0.019993 - 0.000101 * t) * Math.sin(2.0 * rm);
        c = c + 0.00029 * Math.sin(3.0 * rm);
        tl = lo + c;
        v = m + c;
        r = (1.000001018 * (1.0 - e * e)) / (1.0 + (e * Math.cos(this.radians(v))));
        nut = 125.04 - 1934.136 * t;
        al = this.radians(tl - 0.00569 - 0.00478 * Math.sin(this.radians(nut)));
        obliq = 23.4391666666667 - 1.30041666666666E-02 * t - 0.000000163888888 * t * t + 5.03611111111E-08 * t * t * t;
        obliq = this.radians(obliq + 0.00256 * Math.cos(this.radians(nut)));
        sunra = this.degrees(Math.atan2(Math.cos(obliq) * Math.sin(al),Math.cos(al)));
        if (sunra < 0)
            sunra = 360 + sunra;
        sundecl = this.degrees(Math.asin(Math.sin(obliq) * Math.sin(al)));
        s.lat = sundecl;
        s.lng = sunra;
        return(s);
    };

    /* sidereal time from Julian Date */
    this.sidtime = function(jd,lng)
    {
        var t,x;
        t = ((Math.floor(jd) + 0.5) - 2451545.0) *2.73785078713210E-05;
        x = 280.46061837;
        x = x + 360.98564736629 * (jd - 2451545.0);
        x = x + 0.000387933 * t * t;
        x = x - (t * t * t) * 2.583311805734950E-08;
        x = x - lng;
        x = x * this.convcircle;
        x = x - Math.floor(x);
        x = x * 360.0;
        return(x);
    };

    this.rmstime = function(jd,lat,lng,timezone,atcr,ra,decl)
    {
        var x,gmtsid;
        var r = new this.sunRTS();
        r.jd = jd;
        timezone *= this.convday;
        gmtsid = this.sidtime(Math.floor(jd) + 0.5, 0.0);
        r.transit = ((ra + lng - gmtsid) / 360.0);
        atcr = this.radians(atcr);
        lat = this.radians(lat);
        decl = this.radians(decl);
        x = Math.sin(atcr) - (Math.sin(lat) * Math.sin(decl));
        x = x / (Math.cos(lat) * Math.cos(decl));
        if ((x < 1.0) && (x > -1.0))
        {
            x = this.degrees(Math.acos(x)) * this.convcircle;
            r.rise = (this.mod1((r.transit - x)+timezone)) * 24.0;
            r.set = (this.mod1((r.transit + x)+timezone)) * 24.0;
            r.transit = (this.mod1(r.transit+timezone)) * 24.0;
        }
        else
        {
            x = (x >= 0.0)?-100.0:100.0;
            r.rise = x;
            r.transit = x;
            r.set = x;
        }
        return r;
    };

    this.mod1 = function(t)
    {
        var ot;
        ot = t;
        t = Math.abs(t);
        t = t - Math.floor(t);
        if(ot < 0.0)
            t = 1-t;
        return(t);
    };

    this.dt_hms = function(x,amdsp,dst) {
        var h,m,s;
        var sh,sm,ss;
        var ampm;
        var outstr;
        if((x > -100.0) && (x < 100.0)) {
            x = (x < 0.0)?0.0:x;
            h = Math.floor(x);
            x = (x-h) * 60.0;
            if(dst)
                h += 1.0;  // daylight time adjustment
            h %= 24.0;
            m = Math.floor(x);
            x = (x-m) * 60.0;
            s = Math.floor(x);
            if(amdsp) {
                ampm = (h > 11)?"&nbsp;PM":"&nbsp;AM";
                h = h % 12.0;
                h = (h < 1)?12:h;
            }
            else
            {
                ampm = "";
            }
            sh = this.leadChar(h,"0",2);
            sm = this.leadChar(m,"0",2);
            ss = this.leadChar(s,"0",2);
            outstr = sh + ":" + sm + ":" + ss + ampm;
        }
        else
        {
            outstr = (x < 0.0)?"[Below]":"[Above]";
        }
        return outstr;
    };

    this.rmsCheck = function(s, r)
    {
        if((r == -100) || (r == 100))
            return r;
        else
            return s-r;
    };

    this.rms_diff = function(rms) {
        return rmsCheck(rms.set,rms.rise);
    };

    this.compute_current_date = function(lat,lng,year,month,day,tz) {
        var jd = this.mdy_jd(year,month,day,0,0,0);
        var sp = new this.sunRTS();
        var rms = new this.sunRTS();

        sp = this.calcsun(jd,0,0,0,tz);

        rms = this.rmstime(jd,lat,lng,tz,this.SUNSET,sp.lng,sp.lat);
        var sunrise = this.dt_hms(rms.rise,false,false);
        var sunset = this.dt_hms(rms.set,false,false);
        return { "sunrise": sunrise, "sunset": sunset };
    };

    this.format_date = function(date) {
        return date.yr + "-" + this.leadChar(date.mo,'0',2) + "-" + this.leadChar(date.da,'0',2);
    };

    this.longest_day = new this.sunRTS();
    this.shortest_day = new this.sunRTS();
    this.latest_sunrise = new this.sunRTS();
    this.latest_sunset = new this.sunRTS();
    this.earliest_sunrise = new this.sunRTS();
    this.earliest_sunset = new this.sunRTS();
};

function pad(n)
{
    return n < 10 ? '0' + n : n;
};

function update() {
    var suncalc = new SunCalc();
    var latitude = parseFloat(Property.getProperty('/config/geodata/latitude'), 10);
    var longitude = parseFloat(Property.getProperty('/config/geodata/longitude'), 10);
    longitude = -longitude;

    var now = new Date();
    var gmt = -now.getTimezoneOffset()/60;

    if ((latitude !== null) && (longitude !== null)) {
        var solar = suncalc.compute_current_date(latitude,
                                                 longitude,
                                                 now.getFullYear(),
                                                 now.getMonth() + 1,
                                                 now.getDate(),
                                                 gmt);

        print("Calculated solar times for location " + latitude + "/" + longitude + ":");
        for (key in solar) {
            print(" solar." + key + " = " + solar[key]);
        }

        // special handling for possible return value from solar calculator
        if ((solar.sunrise !== "[Above]") && (solar.sunrise !== "[Below]") &&
            (solar.sunset !== "[Above]") && (solar.sunset !== "[Below]")) {

            Property.setProperty('/config/geodata/sunrise', solar.sunrise);
            Property.setProperty('/config/geodata/sunset', solar.sunset);
        }
    }
};

if (raisedEvent.name == "running") {

    var la = Property.getProperty('/config/geodata/latitude');
    var lo = Property.getProperty('/config/geodata/longitude');
    if (la === null && lo === null) {
        // set example location central europe
        Property.setProperty('/config/geodata/latitude', "50.07");
        Property.setProperty('/config/geodata/longitude', "8.4");
    }

    update();

    var d = new Date();
    d.setHours(3);
    d.setMinutes(1);
    var icalST = String(d.getFullYear()) +
                 pad(d.getMonth() + 1) +
                 pad(d.getDate()) + 'T' +
                 pad(d.getHours()) +
                 pad(d.getMinutes()) +
                 '00';
    var icalRR = 'FREQ=DAILY';
    var updateEvent = new TimedICalEvent("solar_computer.update",
                                          icalST, icalRR);
    updateEvent.raise();
} else if (raisedEvent.name == "solar_computer.update") {
    update();
}
