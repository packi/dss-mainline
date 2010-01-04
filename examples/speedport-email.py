#!/usr/bin/python

#
#    Copyright (c) 2009 digitalSTROM.org, Zurich, Switzerland
#
#    Author: Roman Koehler, aizo ag
#
#    This file is part of digitalSTROM Server
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
import imaplib
import re
import time

url = 'http://localhost:8080/dss.wsdl'

print 'starting up speedport - email tests wait 1 minute'
time.sleep(60)
print 'starting up speedport - email tests'

# insert here parameter for requests
mailHost = 'imap.t-online.de'
mailPort = 993
mailUser = 'email'
mailPasswd = 'passwort'

speedportHost = '192.168.0.65'
speedportPort = 1012

requestPeriod = 3

fSpeedportReady = False
fEmailReady = False

fUnseenEmail = False
fTelefonActive = False
fChange=False

unseen = -1
unseenOld = -1


while 1:
	fChange=False
	try:
		dSClient = Client(url);

		# try to get connection to speedport
		if fSpeedportReady == False:
			try:
				telnetSession = telnetlib.Telnet(speedportHost, speedportPort)
				time.sleep(1)
				fSpeedportReady=True
			except:
				print 'no connection to speedport'

		# connection established, so check 
		if fSpeedportReady == True:
			print 'Waiting for dial-event'
			try:
				returnValue =telnetSession.read_until('RING',requestPeriod)
				if len(returnValue) > 5: 
					print 'Got event from: ' + returnValue
					if returnValue.find('RING') > -1:
						print 'got Ring'
						fTelefonActive=True
						fChange=True
					if returnValue.find('DISCONNECT') > -1:
						print 'got disconnect'
						fTelefonActive=False
						fChange=True
				else:
					print 'nothing happens with the speedport'
			except:
				print 'Error querying speedport'
				fSpeedportReady=False
		else:
			# no connection, so wait 
			time.sleep(60 * requestPeriod)

		try:
			server = imaplib.IMAP4_SSL(mailHost, mailPort)
			try:
				server.login(mailUser, mailPasswd)
				ok, resp = server.status('INBOX', '(UNSEEN)')
				if ok == 'OK':
					fEmailReady = True
					unseen = int(re.search('UNSEEN\s+(\d+)',resp[0]).group(1))
					
					if unseen != unseenOld:
						unseenOld=unseen;
						fChange=True
						if unseen > 0:
							print 'Have unseen messages', unseen
							fUnseenEmail=True
						else:
							print 'Have seen messages', unseen
							fUnseenEmail=False
					else:
						print 'Have no change'
				else:
					fEmailReady = False
			finally:
				server.logout()
	        	
		except Exception, inst:
			print 'Error querying mail:', inst
			fEmailReady = False


		if fChange:
			if fSpeedportReady == True:
				if fTelefonActive == True:
					try:
						token = dSClient.service.Authenticate('user', 'password')
						dSClient.service.EventRaise(token, 'call', '', 'source=0', '')
						dSClient.service.SignOff(token)
						print 'Raised event: call'
					except Exception, inst:
						print 'Error sending event:', inst
		
		# when telefon active, then no new email 
		if fTelefonActive == True:
			fChange=False

		if fChange:
			if fEmailReady:
				if fUnseenEmail:				
					try:
						token = dSClient.service.Authenticate('user', 'password')							
						dSClient.service.EventRaise(token, 'unread_mail', '', 'count='+str(unseen), '')
						dSClient.service.SignOff(token)
						print 'Raised event: unread_mail'
					except Exception, inst:
						print 'Error sending event:', inst
				else:
					try:
						token = dSClient.service.Authenticate('user', 'password')
						dSClient.service.EventRaise(token, 'read_mail', '', 'count='+str(unseen), '')
						dSClient.service.SignOff(token)
						print 'Raised event: read_mail'
					except Exception, inst:
						print 'Error sending event:', inst

	except Exception, inst:
		print 'general Error; waiting 1 minute :', inst
		time.sleep(60)
