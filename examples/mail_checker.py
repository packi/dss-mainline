#!/usr/bin/python

from suds.client import Client
import imaplib
import re
import time

url = 'http://localhost:8080/dss.wsdl'
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
