#!/usr/bin/python

#
#    Copyright (c) 2009 digitalSTROM.org, Zurich, Switzerland
#
#    Author: Patrick Staehlin, futureLAB AG <pstaehlin@futurelab.ch>
#
#    This file is part of digitalSTROM Server.
#
#    digitalSTROM Server is free software: you can redistribute it and/or modify
#    it under the terms of the GNU General Public License as published by
#    the Free Software Foundation, either version 3 of the License, or
#    (at your option) any later version.
#
#    digitalSTROM Server is distributed in the hope that it will be useful,
#    but WITHOUT ANY WARRANTY; without even the implied warranty of
#    MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
#    GNU General Public License for more details.
#
#    You should have received a copy of the GNU General Public License
#    along with digitalSTROM Server. If not, see <http://www.gnu.org/licenses/>.
#

from suds.client import Client
import telnetlib
import time

url = 'https://localhost:8080/dss.wsdl'
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

