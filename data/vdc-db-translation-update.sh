#!/bin/sh
#
# Update remote vdc-db with translations from Transifex

GTIN=""
[ -n $1 ] && GTIN="gtin=${1}&"

DSS_LOCALES="de_DE pl_PL fr_CH en_US nl_NL tr_TR ru_RU it_IT es_ES nb_NO pt_PT sv_SE da_DK"

for LANG in $DSS_LOCALES; do
  echo "Update $LANG"
  curl "http://db.aizo.net/nameimport.php?${GTIN}langcode=$LANG"
done
