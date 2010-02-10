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
import imaplib
import re
import time

url = 'https://localhost:8080/dss.wsdl'
mailHost = ''
mailPort = 993
mailUser = ''
mailPasswd = ''
sleepSeconds = 10

client = Client(url);

while 1:
  try:
    server = imaplib.IMAP4_SSL(mailHost, mailPort)
    try:
      server.login(mailUser, mailPasswd)
      ok, resp = server.status('INBOX', '(UNSEEN)')
      if ok == 'OK':
        unseen = int(re.search('UNSEEN\s+(\d+)',resp[0]).group(1))
        if unseen > 0:
          print 'Have unseen messages', unseen
          try:
	    token = client.service.Authenticate('user', 'password')
	    client.service.EventRaise(token, 'unread_mail', '', 'count='+str(unseen), '');
	    client.service.SignOff(token)
	  except Exception, inst:
	    print 'Error sending event:', inst
    finally:
      server.logout()
  except Exception, inst:
    print 'Error:', inst
  time.sleep(sleepSeconds)
