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
import time
import smtplib

url = 'https://localhost:8080/dss.wsdl'
mailserver = 'smtp.changemeserver.somwhere'
mailfrom = 'dss@localhost'
mailto = [ 'jemand@irgendwo.domain' ]
mailsubject = 'dSS comm down'
mailtext = 'It appears that the ds485 communication failed'

message = """\
From: %s
To: %s
Subject: %s

%s
""" % (mailfrom, ", ".join(mailto), mailsubject, mailtext)

client = Client(url);

lastWasError = False

while 1:
  time.sleep(1);


  try:
    token = client.service.Authenticate('user', 'password')
#    print 'Received token'

    state = client.service.PropertyGetString(token, "/system/DS485Proxy/state");
#    print state

    sendMail = False;
    if state == "error":
      if not lastWasError:
        sendMail = True
	lastWasError = True
    elif state == "comm error":
      if not lastWasError:
	lastWasError = True
	sendMail = True
    else:
      if lastWasError:
        print "all ok again"
      lastWasError = False

    if sendMail:
      print "sending email"
      server = smtplib.SMTP(mailserver)
      server.sendmail(mailfrom, mailto, message)
      server.quit()

    client.service.SignOff(token)
  except Exception, inst:
    print 'Error sending event:', inst

