#!/bin/sh

###
## Generate a build info header; unix version
#
# Johannes Winkelmann, johannes.winkelmann@aizo.com

BASEDIR=$1
TARGETDIR=$2
BUILD_INFO_TMP=build_info.h.tmp.$(date +%s)
BUILD_INFO=build_info.h

if [ -z "$TARGETDIR" ] ; then
  echo "Usage: $(basename $0) <base directory> <build_info target_dir>"
  exit 1
fi

rcs_rev="-"
rcs_root="-"
if [ -d $BASEDIR/.git ]; then
  rcs_rev="git:$(git --git-dir=$BASEDIR/.git rev-parse HEAD)"
  rcs_root=$(git --git-dir=$BASEDIR/.git branch|grep ^*|sed -e 's|^\*\s*||')
  
  rcs_st=$(cd $BASEDIR; git status | grep modified:)
  if [ -n "$rcs_st" ]; then
    rcs_rev="$rcs_rev-dirty"
  fi
else
  if [ -f $BASEDIR/.dss_git_version ]; then
    rcs_rev=$(cat $BASEDIR/.dss_git_version)
  fi
  if [ -f $BASEDIR/.dss_git_branch ]; then
    rcs_root=$(cat $BASEDIR/.dss_git_branch)
  fi
fi


echo "/* $BUILD_INFO generated by $(basename $0)        */" \
	> $TARGETDIR/$BUILD_INFO_TMP
echo "/* do not edit, modifications will be overwritten */" \
	>> $TARGETDIR/$BUILD_INFO_TMP


echo "#define DSS_RCS_REVISION \"$rcs_rev\""       >> $TARGETDIR/$BUILD_INFO_TMP
echo "#define DSS_RCS_URL      \"$rcs_root\""      >> $TARGETDIR/$BUILD_INFO_TMP
echo "#define DSS_BUILD_USER   \"$(whoami)\""      >> $TARGETDIR/$BUILD_INFO_TMP
echo "#define DSS_BUILD_HOST   \"$(hostname)\""    >> $TARGETDIR/$BUILD_INFO_TMP

# check whether the actual contents changed; don't overwrite otherwise
if [ -f $TARGETDIR/$BUILD_INFO ]; then
    if [ -z "$(diff -q $TARGETDIR/$BUILD_INFO_TMP $TARGETDIR/$BUILD_INFO)" ]; then
	echo "Info: No changes since last $BUILD_INFO generation"
	rm $TARGETDIR/$BUILD_INFO_TMP
	exit 0
    fi
fi    
mv $TARGETDIR/$BUILD_INFO_TMP $TARGETDIR/$BUILD_INFO
