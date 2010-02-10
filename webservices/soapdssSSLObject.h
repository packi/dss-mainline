/*
    Copyright (c) 2009 digitalSTROM.org, Zurich, Switzerland

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

#ifndef __SOAP_DSS_SSL_SERVICE_H__
#define __SOAP_DSS_SSL_SERVICE_H__

#include "soapH.h"
#include "soapdssObject.h"

// It seems that gsoap does not generate the C++ methods that are needed
// for SSL, we fix that by subclassing the generated code and by adding these
// functions manually.
class SOAP_CMAC dssSSLService : public dssService
{ public:
    dssSSLService();

    int ssl_server_context(unsigned short, const char*, 
            const char*, const char*, const char*, const char*, 
            const char*, const char*);

    virtual ~dssSSLService();
};
#endif
