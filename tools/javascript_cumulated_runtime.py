#!/usr/bin/env python
"""
foo
"""

import json
import sys

# check with
# pylint --reports=no --notes="" tools/json_timings2csv.py

def usage():
    """
    usage
    """
    print ""
    print " SYNTAX: javascript_cumulated_runtime.py <filename>"
    print ""
    print " where filename is the output created with"
    print " dss_json.sh '/json/property/query2?query=/system/js/timings(*)/*(*)/*(*)'"
    print " as typically found on statistics server under uploaded_files/javascript"
    print ""

if len(sys.argv) < 2:
    usage()
    sys.exit(-1)

JSON_DATA = open(sys.argv[1])

DATA = json.load(JSON_DATA)

if DATA['ok'] != True:
    print "%s corrupt file" % JSON_FILE
    sys.exit(-2)

TIMINGS = DATA['result']['timings']
UPDATE = TIMINGS['updated']
del TIMINGS['updated']

"""
" query2 format
"   { subs1: { count : val2, total: time, pre: time, post: time, script1: {totalTime: time}}}
"""

K_TIME = "total"
K_COUNT = 'count'
K_PRE = 'pre'
k_POST = 'post'

print "    ms    count  subscription"
for subs in reversed(sorted(TIMINGS, key=lambda subs: int(TIMINGS[subs][K_TIME]))):
    print "%6d %8d  %s" % ((TIMINGS[subs][K_TIME] / 1000.0), TIMINGS[subs][K_COUNT], subs)
