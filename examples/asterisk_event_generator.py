#!/usr/bin/python

#
#    Copyright (c) 2009 digitalSTROM.org, Zurich, Switzerland
#    Copyright (c) 2009 futureLAB AG, Winterthur, Switzerland
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

url = 'http://localhost:8080/dss.wsdl'
asteriskHost = 'asterisk.futurelab.ch'
asteriskPort = 5038

client = Client(url);

tn = telnetlib.Telnet(asteriskHost, asteriskPort)
time.sleep(1)
tn.write("Action: Login\r\nActionID: 1\r\nUsername: dss\r\nSecret: Kae%woh6\r\n\r\n")
tn.read_until('Response: Success')

while 1:
  print 'Waiting for dial-event'
  tn.read_until('Event: Dial')

  tn.read_until('Destination:')
  dest = tn.read_until('\r\n')
  if dest.find('patton') == -1:
    print 'Got event'

    tn.read_until('CallerID: ');
    number = tn.read_until('\r\n').strip();
    print 'From: "' + number + '"'

    dest = dest.strip()
    print 'To:   "' + dest[4:8] + '"'

    try:
      token = client.service.Authenticate('user', 'password')
      print 'Received token'

      client.service.EventRaise(token, 'call', '', 'source='+number+';destination='+dest[4:8], '');
      print 'Raised event.'

      client.service.SignOff(token)
    except:
      print 'Error sending event, dSS down?'

