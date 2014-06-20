#!/usr/bin/env python3

import urllib3;
import os;

os.system('pkill -12 dss')

#url_authority = "http://dss-kit.local:8088"
#url_authority = "https://dss-kit.local:8080"
url_authority = "https://dss-kit.local:443"
#url_authority = "http://127.0.0.1:8088"
#url_authority = "https://127.0.0.1:8080"

if url_authority.startswith('https'):
    http_pool = urllib3.connection_from_url(url_authority, cert_reqs='CERT_NONE', ca_certs=None)
else:
    http_pool = urllib3.connection_from_url(url_authority)

url_login = "/json/system/login?user=dssadmin&password=dssadmin"
url_subscribe = "/json/event/subscribe?subscriptionID=38505001832&name=test-addon.answer&a=0.4565491367010114&"
url_unsubscribe= '/json/event/unsubscribe?subscriptionID=38505001832&name=test-addon.answer&a=0.46161272635681416&'

def login():
    login_header = { "Authorization" : "username=\"dssadmin\"" }
    r = http_pool.urlopen('GET', url_login, None, login_header)
    token = (r.headers['set-cookie'].split(';')[0])
    return token

headers = urllib3.make_headers(keep_alive=True, user_agent="json-tester/1.0", accept_encoding=True)
token = login()
headers["cookie"]  = token

for i in range(1000):
    r = http_pool.urlopen('GET', url_subscribe, None, headers)
    print(r.data)
    r = http_pool.urlopen('GET', url_unsubscribe, None, headers)
    print(r.data)

os.system('pkill -12 dss')
