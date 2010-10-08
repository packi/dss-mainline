/*
    Copyright (c) 2010 digitalSTROM.org, Zurich, Switzerland

    Author: Sergey 'Jin' Bostandzhyan <jin@dev.digitalstrom.org> 

    This file is part of digitalSTROM Server.

    digitalSTROM Server is free software: you can redistribute it and/or modify
    it under the terms of the GNU General Public License as published by
    the Free Software Foundation, either version 3 of the License, or
    (at your option) any later version.

    digitalSTROM Server is distributed in the hope that it will be useful,
    but WITHOUT ANY WARRANTY; without even the implied warranty of
    MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
    GNU General Public License for more details.

    You should have received a copy of the GNU General Public License
    along with digitalSTROM Server. If not, see <http://www.gnu.org/licenses/>.

*/

#ifdef HAVE_CONFIG_H
  #include "config.h"
#endif

#include "soapdssSSLObject.h"
#include "gs-locks.h"
#include "core/logger.h"

dssSSLService::dssSSLService() : dssService() 
{
  CRYPTO_setup_locks();
  soap_init(this);
  bind_flags = SO_REUSEADDR;
  accept_timeout = SOAP_ACCEPT_TIMEOUT;
}

dssSSLService::~dssSSLService()
{
  soap_done(this);
  CRYPTO_cleanup_locks(); 
}

int dssSSLService::dssSSLService::ssl_server_context(unsigned short param, 
        const char* serverpem, const char* password, 
        const char* cacertpem, const char* capath, 
        const char* dhfile, const char* randfile, const char* serverid)
{
  int i;
  i = soap_ssl_server_context(this, param, serverpem, password, cacertpem, 
                              capath, dhfile, randfile, serverid);

  if (i) {
    soap_print_fault(this, stderr);
  }

  return i;

}
