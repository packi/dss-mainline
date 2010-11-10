#!/bin/bash

which openssl >/dev/null
if [ "$?" -ne "0" ]; then
    echo "Please install openssl and make sure it is in your PATH!"
    exit 1
fi

if [ ! -f "privkey.pem" ]; then
    openssl genrsa -out privkey.pem 1024
    if [ "$?" -ne "0" ]; then
        echo "Failed to generate private key!"
        exit 1
    fi
    chmod 400 privkey.pem
else
    echo "Reusing existing privkey.pem file as private key."
fi

if [ ! -f "certreq.csr" ]; then
    openssl req -new -key privkey.pem -out certreq.csr
    if [ "$?" -ne "0" ]; then
        echo "Failed to create a certificate signing request!"
        exit 1
    fi
else
    echo "Reusing existing certreq.csr file as certificate signing request."
fi

openssl x509 -req -in certreq.csr -signkey privkey.pem -out dsscert.pem
if [ "$?" -ne "0" ]; then
    echo "Could not create self signed certificate!"
    exit 1
fi

cat privkey.pem >> dsscert.pem
if [ "$?" -ne "0" ]; then
    echo "Failed to add privkey.pem to dsscert.pem"
    exit 1
fi

echo "Certificate created as \"dsscert.pem\""

