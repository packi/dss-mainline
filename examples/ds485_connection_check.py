#!/usr/bin/python

from suds.client import Client
import time
import smtplib

url = 'http://localhost:8080/dss.wsdl'
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

