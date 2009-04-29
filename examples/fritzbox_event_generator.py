#!/usr/bin/python

from suds.client import Client
import telnetlib
import time

url = 'http://localhost:8080/dss.wsdl'
fritzBoxHost = 'fritz.box'
fritzBoxPort = 1012
filter = None

client = Client(url);

tn = telnetlib.Telnet(fritzBoxHost, fritzBoxPort)
time.sleep(1)

while 1:
  print 'Waiting for dial-event'
  tn.read_until('RING')
  line = tn.read_until('\n')
  number = line.split(';')[2]
  if filter != None:
    if number.find(filter) == -1:
      continue

  print 'Got event from: ' + number

  try:
    token = client.service.Authenticate('user', 'password')
    print 'Received token'

    client.service.EventRaise(token, 'call', '', 'source='+number, '');
    print 'Raised event.'

    client.service.SignOff(token)
  except:
    print 'Error sending event, dSS down?'

