
var c = new easycurl;

for (var i in c) {
    print("" + i + ": " + c[i]);
}

var blob = "";

c.header = function(data) {
    print("HEADER: " + data.trim());
}

c.write = function(data) {
    print("DATA WRITE: " + data.length);
}

c.debug = function debug(itype, data) {
    switch (itype) {
        case 0:
            // informational messages
            print(data.trim());
            break;
        case 1:
            // response headers
            break;
        case 2:
            // request headers
            //print(ByteArray(data).decodeToString('ascii').trim());
            break;
        case 3:
            // data in?
            break;
        case 4:
            // data out?
            break
        default:
            print("UNKNOWN");
    }
}

var headers = new easylist;
headers.append('Accept: text/html,application/xhtml+xml,application/xml;q=0.9,*/*;q=0.8');
headers.append('Accept-Language: en-us,en;q=0.5');
headers.append('Username: dssadmin');
print("headers: " + headers.toArray());
c.setopt(c.CURLOPT_HTTPHEADER, headers);

// change this to "1" to show everything
c.setopt(c.CURLOPT_VERBOSE, 0);

c.setopt(c.CURLOPT_HTTPAUTH, 2);
c.setopt(c.CURLOPT_USERNAME, "dssadmin");
c.setopt(c.CURLOPT_PASSWORD, "dssadmin");

// SSL: don't bother with peer verification
c.setopt(c.CURLOPT_SSL_VERIFYPEER, 0);
// on redirect, follow it
c.setopt(c.CURLOPT_FOLLOWLOCATION, 1);
// but don't follow too may times.
c.setopt(c.CURLOPT_MAXREDIRS, 10);
// and update the referrer http header automatically
c.setopt(c.CURLOPT_AUTOREFERER, 1);

c.setopt(c.CURLOPT_HTTPGET, 1);

/*
c.setopt(c.CURLOPT_URL, 'https://10.0.37.100/json/system/login?user=dssadmin&password=dssadmin');
result = c.perform();
print(result);
*/

/*
json = eval(result);
if (json.ok) {
    token = json.result.token;
}
c.setopt(c.CURLOPT_COOKIE, "Token:" + token);
*/

c.setopt(c.CURLOPT_URL, 'https://10.0.37.100/json/zone/callScene?id=1241&groupID=1&sceneNumber=0');
result = c.perform();
print(result);
