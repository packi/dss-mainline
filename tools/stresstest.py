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

import datetime

url = 'http://localhost:8080/dss.wsdl'

print 'stresstest.py'
while 1:
  print 'creating client'
  client = Client(url);
  try:
    print 'getting token'
    token = client.service.Authenticate('user', 'pass')
    i = 0;
    while 1:
      print 'Setting property'
      client.service.PropertySetInt(token, '/sim/3504175fe0000000ffc00016/temperature', i);
      i += 1;
      time.sleep(0.5);
  except Exception, inst:
    print 'error ', inst
