#!/bin/sh

SEPAR="============================================================"
svn_revision_file="celestia-svn-revision"

svn_old_rev=`cat ${svn_revision_file}`
cd work/src
svn_new_rev=`svn info | grep Revision | awk '{print $2}'`
cd ../

echo $SEPAR
echo " Merging celestia's sources"
echo " Old revision: $svn_old_rev"
echo " New revision: $svn_new_rev"
echo $SEPAR
echo " Make sure that no there no conflicts and"
echo -n ' press the [Enter] or [Return] key to continue '
read DISCARD

find src/cel3ds/  -name "*.cpp" -exec cp {} ../../src/cel3ds \;
find src/cel3ds/  -name "*.h" -exec cp {}  ../../src/cel3ds \;

find src/celengine/ -depth 1 -name "*.cpp" -exec cp {} ../../src/celengine \;
find src/celengine/ -depth 1 -name "*.h" -exec cp {} ../../src/celengine \;

find src/celestia/ -depth 1 -name "*.cpp" -exec cp {} ../../src/celestia \;
find src/celestia/ -depth 1 -name "*.h" -exec cp {} ../../src/celestia \;

find src/celmath/ -depth 1 -name "*.cpp" -exec cp {} ../../src/celmath \;
find src/celmath/ -depth 1 -name "*.h" -exec cp {} ../../src/celmath \;

find src/celtxf/ -depth 1 -name "*.cpp" -exec cp {} ../../src/celtxf  \;
find src/celtxf/ -depth 1 -name "*.h" -exec cp {} ../../src/celtxf  \;

find src/celutil/ -depth 1 -name "*.cpp" -exec cp {} ../../src/celutil \;
find src/celutil/ -depth 1 -name "*.h" -exec cp {} ../../src/celutil \;


find src/ -depth 1 -name "*.cpp" -exec cp {} ../../src \;
find src/  -depth 1 -name "*.h" -exec cp {}  ../../src \;

cp -rf  src/tools ../../src/

cp -rf thirdparty ../../

echo $svn_new_rev > ../${svn_revision_file}
echo "Done."

