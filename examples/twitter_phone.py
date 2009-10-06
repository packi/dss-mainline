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

# Uses twitter API from http://mike.verdone.ca/twitter/
from twitter.api import Twitter, TwitterError
import datetime

url = 'http://localhost:8080/dss.wsdl'
twitterEmail = 'your@email.com'
twitterPassword = 'yourPassword'

def lookupNumber(number):
  if number == '0791234567':
    return 'Patrick'
  return number

while 1:
  client = Client(url);
  try:
    while 1:
      token = client.service.Authenticate('user', 'pass')
      client.service.EventSubscribeTo(token, 'phone')
      print 'Waiting for event'
      events = client.service.EventWaitFor(token, 0)
      print events
      caller = 'unknown'
      for event in events:
        for parameter in event.parameter:
	  key,value = parameter.split('=',1)
	  if key == 'source':
	    caller = lookupNumber(value)
	    
      print 'Received event'
      twitter = Twitter(twitterEmail, twitterPassword, agent='dssTwitter')
      t = datetime.datetime.now()
      stat = t.strftime('%d.%m.%Y %H:%M') + ' => call from ' + caller
      print stat
      twitter.statuses.update(status=stat)
      client.service.SignOff(token)
  except Exception, inst:
    print 'error ', inst
