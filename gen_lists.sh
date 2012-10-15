FILE="build/sources.list"

echo -n 'libdsscore_a_SOURCES = ' > $FILE

for f in `find src | egrep '\.(cpp?|h)$' | egrep -v 'build_info.h' | sort`
do
    if (test -f $f); then
        echo " \\" >> $FILE
        echo -n '	$(top_srcdir)/' >>$FILE
        echo -n $f >> $FILE
    fi
done

echo >> $FILE
echo >> $FILE

echo -n "libmongoose_a_SOURCES =" >> $FILE
for f in `find external/mongoose | egrep '\.(c?|h)$' | sort`
do
    if (test -f $f); then
        echo " \\" >> $FILE
        echo -n '	$(top_srcdir)/' >>$FILE
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
        echo -n '	$(top_srcdir)/' >>$FILE
        echo -n $f >> $FILE
    fi
done

echo >> $FILE
echo >> $FILE

echo -n "libwebservices_a_SOURCES =" >> $FILE
for f in `find webservices | egrep '\.(cpp?|h)$' | egrep -v '(soapH.h|soapC.cpp|soapServer.cpp|soapStub.h|soapdssObject.h)' | sort`
do
    if (test -f $f); then
        echo " \\" >> $FILE
        echo -n '	$(top_srcdir)/' >>$FILE
        echo -n $f >> $FILE
    fi
done

echo >> $FILE
echo >> $FILE

echo -n "dsstests_SOURCES =" >> $FILE
for f in `find tests | egrep '\.(cpp?|h)$' | egrep -v '(dsidhelpertest.cpp|dssimtest.cpp)'| sort`
do
    if (test -f $f); then
        echo " \\" >> $FILE
        echo -n '	$(top_srcdir)/' >>$FILE
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
        echo -n '   $(top_srcdir)/' >>$IMAGES_FILE
        echo -n $f >> $IMAGES_FILE
    fi
done

echo >> $IMAGES_FILE
echo >> $IMAGES_FILE


