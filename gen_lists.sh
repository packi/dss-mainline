FILE="build/sources.list"

echo -n 'libdsscore_a_SOURCES = ' > $FILE

for f in `find src | egrep '\.(cpp?|h)$' | egrep -v '(messages|build_info.h)' | sort`
do
    if (test -f $f); then
        echo " \\" >> $FILE
        echo -n '	../' >>$FILE
        echo -n $f >> $FILE
    fi
done

echo >> $FILE
echo >> $FILE

echo -n "libcivetweb_a_SOURCES =" >> $FILE
for f in `find external/civetweb | egrep '\.(c?|h|inl)$' | sort`
do
    if (test -f $f); then
        echo " \\" >> $FILE
        echo -n '	../' >>$FILE
        echo -n $f >> $FILE
    fi
done

echo >> $FILE
echo >> $FILE

echo -n "libunix_a_SOURCES =" >> $FILE
for f in `find unix | egrep '\.(cpp?|h)$' | sort`
do
    if (test -f $f); then
        echo " \\" >> $FILE
        echo -n '	../' >>$FILE
        echo -n $f >> $FILE
    fi
done

echo >> $FILE
echo >> $FILE

echo -n "dsstests_SOURCES =" >> $FILE
for f in `find tests | egrep '\.(cpp?|h)$' | egrep -v '(dssimtest.cpp)'| sort`
do
    if (test -f $f); then
        echo " \\" >> $FILE
        echo -n '	../' >>$FILE
        echo -n $f >> $FILE
    fi
done

echo >> $FILE
echo >> $FILE

IMAGES_FILE=data/images.list

echo -n "dist_dssdataimages_DATA = " > $IMAGES_FILE
for f in `find data/images | egrep '\.(png)$' | sort`
do
    if (test -f $f); then
        echo " \\" >> $IMAGES_FILE
        echo -n '   ../' >>$IMAGES_FILE
        echo -n $f >> $IMAGES_FILE
    fi
done

echo >> $IMAGES_FILE
echo >> $IMAGES_FILE


