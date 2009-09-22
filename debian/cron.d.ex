#
# Regular cron jobs for the dss package
#
0 4	* * *	root	[ -x /usr/bin/dss_maintenance ] && /usr/bin/dss_maintenance
