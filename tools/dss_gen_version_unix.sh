#!/bin/sh

###
## Generate a build info header; unix version
#
# Johannes Winkelmann, johannes.winkelmann@aizo.com

BASEDIR=$1
TARGETDIR=$2
BUILD_INFO=build_info.h

if [ -z "$TARGETDIR" ] ; then
  echo "Usage: $(basename $0) <base directory> <build_info target_dir>"
  exit 1
fi

rcs_rev="undefined"
rcs_root="undefined"
if [ -n "$(which svn)" ]; then
  if [ -d $BASEDIR/.svn ]; then
     rcs_rev=$(svn info $BASEDIR|grep ^Revision:| sed -e 's|Revision:\s*||')
     rcs_root=$(svn info $BASEDIR|grep ^URL:|sed -e 's|URL:\s*||')
     rcs_st=$(svn st $BASEDIR|grep ^M)
     if [ -n "$rcs_st" ]; then
	rcs_rev="$rcs_rev-dirty"
     fi
  elif [ -d $BASEDIR/.git ]; then
    if [ -d $BASEDIR/.git/svn ]; then
     rcs_rev=svn-$(git svn info|grep ^Revision:| sed -e 's|Revision:\s*||')+
     rcs_root=$(git svn info|grep ^URL:|sed -e 's|URL:\s*||')    
    else
      rcs_root=$(git branch|grep ^*)
    fi
    rcs_rev="${rcs_rev}git:$(git rev-parse HEAD)"
    rcs_st=$(git status $BASEDIR|grep modified:)
    if [ -n "$rcs_st" ]; then
      rcs_rev="$rcs_rev-dirty"
    fi
  elif [ -d $BASEDIR/.hg ]; then
     rcs_rev=$(hg identify) # already contains the '+' marked when modified
     rcs_root=$(hg branch)
  fi 
fi


echo "/* $BUILD_INFO generated by $(basename $0)        */" \
	> $TARGETDIR/$BUILD_INFO
echo "/* do not edit, modifications will be overwritten */" \
	>> $TARGETDIR/$BUILD_INFO


echo "#define DSS_RCS_REVISION \"$rcs_rev\""    >> $TARGETDIR/$BUILD_INFO
echo "#define DSS_RCS_URL      \"$rcs_root\""   >> $TARGETDIR/$BUILD_INFO
echo "#define DSS_BUILD_USER   \"$(whoami)\""   >> $TARGETDIR/$BUILD_INFO
echo "#define DSS_BUILD_HOST   \"$(hostname)\"" >> $TARGETDIR/$BUILD_INFO
echo "#define DSS_BUILD_DATE   \"$(date)\""     >> $TARGETDIR/$BUILD_INFO
