#!/bin/sh

PROT="http"
SERVER="127.0.0.1:8088"
COOKIES="/tmp/dss-cookies.txt"
OUTPUT="output.json"

USER="dssadmin"
PASSWD=""

# TODO rewrite in C, to overcome differences in wget interface
# TODO when running local, it seems the password is not cheched
# TODO user still has to enter /json/ prefix in url
# TODO check if wget is busybox wget and remove -e option

usage()
{
  echo ""
  echo "SYNTAX: dss_json.sh -s -v -n -d server:port -p password -o output.json url"
  echo " -s  use https instead of http"
  echo "     use if your server listen property has no 'h' at the end"
  echo "     e.g. /config/subsystems/WebServer/listen=127.0.0.1:8080h"
  echo " -d  ip:port"
  echo "     default is localhost:8088"
  echo "     check property '/config/subsystems/WebServer/listen'"
  echo " -p  password"
  echo "     if not set, trusted port is assumed"
  echo " -o  output file defaults to ${OUTPUT}"
  echo " -n  no check certificate, enable if you see this:"
  echo "     'GnuTLS: A TLS packet with unexpected length was received.'"
  echo "     embedded wget does not have this so not enabled by default"
  echo " -v  verbose"
  echo " -e  enable options not support by busybox wget"
  echo "     limit number of retries if a fetch fails"
  echo ""
  echo "url, e.g. '/json/property/query?query=/system/js/timings/*(count,time)/*(*)'"
  echo "all options must come in front of url, incl. output"
  echo ""
  echo "Troubleshoot:"
  echo "1. Are you running on the same box -> use trusted port, no options needed"
  echo "   dss_json query"
  echo "2. Running remote -> you propably need"
  echo "   - setting a password with -p"
  echo "   - https instead of http -s"
  echo "   - enable no check certificate -n, see comment about debian"
  echo "   dss_json -snp <secret> -d ip:port query"
  echo ""
  echo ""
}

while getopts "sd:p:o:vne" opt; do
  case $opt in
    s)
      PROT="https"
      ;;
    d)
      SERVER=$OPTARG
      ;;
    p)
      PASSWD=$OPTARG
      ;;
    o)
      OUTPUT=$OPTARG
      ;;
    n)
      NO_CERT_CHECK=1
      ;;
    v)
      VERBOSE=1
      ;;
    e)
      EXTENDED=1
      ;;
    \?|*)
      usage
      exit
    ;;
  esac
done


shift $(($OPTIND - 1))
QUERY=$1

if [ -z "${QUERY}" ]; then
  usage
  exit
fi


WGET_CMD="wget"
if [ $VERBOSE ]; then
  WGET_CMD="${WGET_CMD} -v"
else
  WGET_CMD="${WGET_CMD} -q"
fi
if [ $EXTENDED ]; then
  WGET_CMD="${WGET_CMD} --tries=1"
fi

if [ $NO_CERT_CHECK ]; then
  # won't fix it for debian stable
  # http://bugs.debian.org/cgi-bin/bugreport.cgi?bug=686219
  WGET_CMD="${WGET_CMD} --no-check-certificate"
fi

fetch_trusted()
{
  WGET_CMD="${WGET_CMD} --header Authorization:username=\"${USER}\""
  ${WGET_CMD} ${PROT}://${SERVER}${QUERY} -O ${OUTPUT}
}

fetch_with_login()
{
  WGET_CMD="${WGET_CMD} --load-cookies ${COOKIES} --keep-session-cookies --save-cookies ${COOKIES}"
  LOGIN_CMD="${WGET_CMD} ${PROT}://${SERVER}/json/system/login?user=${USER}&password=${PASSWD}"
  if [ $VERBOSE ]; then
    echo ${LOGIN_CMD}
  fi
  ${LOGIN_CMD} -O /dev/null
  if [ $? -ne 0 ]; then
    echo "login failed check server and http vs. https"
    exit 1
  fi

  QUERY_CMD="${WGET_CMD} ${PROT}://${SERVER}"${QUERY}" -O ${OUTPUT}"
  if [ $VERBOSE ]; then
    echo ${QUERY_CMD}
  fi
  ${QUERY_CMD}
  if [ $? -ne 0 ]; then
    echo "query failed"
    exit 2
  fi
}

execute()
{
  if [ -z "$PASSWD" ]; then
    echo "Trusted port no login needed"
    fetch_trusted
  else
    fetch_with_login
  fi
  echo "Result saved to ${OUTPUT}"
}

execute
