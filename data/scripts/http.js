/*
 *  This program is free software: you can redistribute it and/or modify
 *  it under the terms of the GNU General Public License as published by
 *  the Free Software Foundation, either version 3 of the License, or
 *  (at your option) any later version.
 *
 *  This program is distributed in the hope that it will be useful,
 *  but WITHOUT ANY WARRANTY; without even the implied warranty of
 *  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 *  GNU General Public License for more details.
 *
 *  You should have received a copy of the GNU General Public License
 *  along with this program.  If not, see <http://www.gnu.org/licenses/>.
 *
 *  Copyright (c) 2012 digitalSTROM.org, Zurich, Switzerland
 *  Author: Michael Troß <michael.tross@aizo.com>
 *
 *  Copyright (c) 2013 digitalSTROM.org, Zurich, Switzerland
 *  Author: Sergey Bostandzhyan <jin@dev.digitalstrom.org>
 *
 *  Based on the gpsee module written by Wes Garland <wes@page.ca>,
 *  http://code.google.com/p/gpsee/
 */
/**
 * HTTP(S)
 *
 *  var extraheaders = [];
 *  extraheaders['User-Agent'] = 'dSS digitalStrom Server';
 *
 *  var h = new HTTP( {
 *     'cookieFile' : "",
 *     'username' : "",
 *     'password' : "",
 *     'authentication' : 3,       // 1 = basic, 2 = digest
 *     'noRedirect': 0,
 *     'maxRedirects' : 12,
 *     'noPeerVerification' : 1,   // don't bother about ssl peer verification
 *     'debug' : 0                 // enable console debug log
 *  },
 *  extraheaders);
 *
 *  var data = h.get('http://www.digitalstrom.org');
 *
 */

var HTTP = function HTTP(options, extraHeaders)
{
  this._easy = new easycurl();
  var headerName;

  if (!options)
    options = {};

  /* Basic options */
  if (options.cookieFile)
    this._easy.setopt(this._easy.CURLOPT_COOKIEFILE, '');
  else
    this._easy.setopt(this._easy.CURLOPT_COOKIEFILE, options.cookieFile + "");

  if (!options.noRedirect)
  {
    this._easy.setopt(this._easy.CURLOPT_FOLLOWLOCATION, 1);
    if (options.maxRedirects)
      this._easy.setopt(this._easy.CURLOPT_MAXREDIRS, +options.maxRedirects);
    else
      this._easy.setopt(this._easy.CURLOPT_MAXREDIRS, 10);

    if (!options.noRedirectReferer)
      this._easy.setopt(this._easy.CURLOPT_AUTOREFERER, 1);
  }

  if (options.noPeerVerification) {
    this._easy.setopt(this._easy.CURLOPT_SSL_VERIFYPEER, 0);
  }

  if (options.username) {
    this._easy.setopt(this._easy.CURLOPT_USERNAME, options.username);
  }

  if (options.password) {
    this._easy.setopt(this._easy.CURLOPT_PASSWORD, options.password);
  }

  if (options.authentication) {
    this._easy.setopt(this._easy.CURLOPT_HTTPAUTH, +options.authentication);
  }

  /* Callbacks for reads and headers */
  this._easy._blobs 	= [];
  this._easy._header_list = [];
  this._easy.write = function easycurl$$write(s)
  {
    //print("GOT CHUNK " + s.length);
    this.blobs.push(s);
  }
  this._easy.header = function easycurl$$header(s)
  {
    //print("GOT HEADER " + s.length + ": ", s.trim());
    this.header_list.push(s);
  }
  var scope = this;
  this._easy.asyncdone = function easycurl$$asyncdone(success)
  {
      if (this.async_callback)
      {
          if (!success)
          {
              var code = { 'success': false };
              this.async_callback(code);
              return;
          }
          this.async_callback(scope._process(scope));
      }
  }

  this._easy.setopt(this._easy.CURLOPT_VERBOSE, options.debug ? 1 : 0);
  this._easy.debug = function easycurl$$debug(itype, data)
  {
    switch (itype)
    {
      case 0: /* informational messages */
        print(data.trim());
        break;
      case 1: /* response headers */
        print('Response: ' + data.trim());
        break;
      case 2: /* request headers */
        print('Request: ' + data.trim());
        break;
      case 3: /* data in? */
        break;
      case 4: /* data out? */
        break;
      default:
        throw new Error("Unknown itype " + itype);
    }
  }

  /*  Must be called at the beginning of each request */
  this.reset  = function(s)
  {
    this._easy.blobs = [];
    this._easy.header_list = [];
  }

  if (typeof options.userAgent !== "undefined")
  {
    if (options.userAgent)
      this._easy.setopt(this._easy.CURLOPT_USERAGENT, options.userAgent);
    else
      this._easy.setopt(this._easy.CURLOPT_USERAGENT, "digitalSTROM Server curl scripting HTTP");
  }

  if (typeof options.encoding !== "undefined")
  {
    if (options.encoding)
      this._easy.setopt(this._easy.CURLOPT_ENCODING, options.encoding);
    else
      this._easy.setopt(this._easy.CURLOPT_ENCODING, 'gzip,deflate');
  }

  this.extraHeaders = extraHeaders;
}

HTTP.prototype =
{
  get: function get(url, referrer, callback)
  {
    this._easy.setopt(this._easy.CURLOPT_HTTPGET, 1);
    this._easy.setopt(this._easy.CURLOPT_POST, 0);
    return this._setup(url, referrer, undefined, callback);
  },

  /**
   * post data must be formatted appropriated into one string
   */
  post: function post(url, referrer, post, callback)
  {
    this._easy.setopt(this._easy.CURLOPT_HTTPGET, 0);
    this._easy.setopt(this._easy.CURLOPT_POST, 1);
    this._easy.setopt(this._easy.CURLOPT_COPYPOSTFIELDS, post);
    return this._setup(url, referrer, undefined, callback);
  },

    /* Merge per-connection and per-method extra headers */
  _process: function(scope)
    {

    var code = 100;
    var i = 0;
    /* code http 100 is a bit funny
     * http://www.w3.org/Protocols/rfc2616/rfc2616-sec10.html
     */
    while (code === 100)
    {
      var status = scope._easy.header_list[i].trim();
      if (status.length > 0)
      {
        /** @todo 	maybe use this._easy.getinfo(this._easy.CURLINFO_RESPONSE_CODE)
         *  		instead of the regexp
         */
        var parts = /^HTTP\/[0-9.]+ ([0-9]+)/(status);
        code = parseInt(parts[1]);
      }
      ++i; /**@todo future: break if i > XXX? */
    }

    var parts = this._easy.blobs.length;
    if (parts == 0)
    {
      this.body = "";
    }
    else
    {
      scope.body = scope._easy.blobs[0];
      for (var i=1; i< parts; ++i)
        scope.body += scope._easy.blobs[i];
    }

    scope.headers = scope._easy.header_list.splice(0);
    if (scope.headers[scope.headers.length - 1] === "\r\n")
      scope.headers.pop();

    var ret = { body: scope.body, status: {}, 'success': true };
    var statusStr = scope.headers.shift() + "";
    ret.status.valueOf 	= function() { return +code };
    ret.status.toString = function() { return statusStr };
    ret.headers = {};
    scope.headers.forEach(function(val)
        {
            var a=val.trim().split(/: /);
            ret.headers[a[0].toLowerCase()] = a[1];
        });

    if (ret.headers['content-type'])
    {
      ret.contentType 	= (ret.headers['content-type'].match(/[a-zA-Z0-9_/-]*/) + "").toLowerCase();
      ret.charset 	= (ret.headers['content-type'].match(/charset=[a-zA-Z0-9_/-]*/) + "").toLowerCase().slice(8);
    }

    ret.valueOf = ret.status.valueOf;
    ret.body.toString = function()
    {
      if (!ret.charset)
        throw new TypeError("Cannot convert content with undefined charset");

      //return ret.body.decodeToString(ret.charset);
      return ret.body;
    }

    return ret;
  },

  _setup: function _setup(url, referrer, headers, callback)
  {
    /* Merge per-connection and per-method extra headers */
    var extraHeaders = [];
    if (headers == undefined) {
      extraHeaders = this.extraHeaders;
    } else {
      extraHeaders = headers.concat(this.extraHeaders);
    }
    if (extraHeaders)
    {
      this.extraHeader_slist = new easylist();
      for (headerName in extraHeaders) {
        //print("add header: ", headerName + ": " + extraHeaders[headerName]);
        this.extraHeader_slist.append(headerName + ": " + extraHeaders[headerName]);
      }
    }

    /** @todo	I suspect _slist represents a core leak. If not, a GC hazard */
    if (this.extraHeader_slist)
      this._easy.setopt(this._easy.CURLOPT_HTTPHEADER, this.extraHeader_slist);

    this._easy.setopt(this._easy.CURLOPT_URL, url);
    this._easy.setopt(this._easy.CURLOPT_REFERER, referrer || "");
    this.reset();
    if (callback == undefined) {
      this._easy.perform();
      return this._process(this);
    } else {
      this._easy.async_callback = callback;
      this._easy.perform_async(callback);
      return null;
    }
  },

  /**
   * Sample to show off the this._easy.getinfo method
   */
  info: function info()
  {
    print(this._easy.getinfo(this._easy.CURLINFO_SIZE_DOWNLOAD) +
	  " bytes downloaded in " +
	  + this._easy.getinfo(this._easy.CURLINFO_TOTAL_TIME) + " seconds");
  }
};
