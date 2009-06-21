#!/bin/sh
#
# Use this script for prepea merging celestia's source files

SEPAR="============================================================"
svn_revision_file="celestia-svn-revision"
rm -rf ./work
mkdir ./work

echo $SEPAR
echo " Checkout celestia/src/"
echo $SEPAR
cd work

svn_rev=`cat ../${svn_revision_file}`
echo Checkout last revision - $svn_rev
svn co http://celestia.svn.sourceforge.net/svnroot/celestia/trunk/celestia/src -r $svn_rev

# Copy celestia sources to work dir and replace
# If this list are changed plz change in commit-svn.sh too 
find ../../src/cel3ds/ -name "*.cpp" -exec cp {} src/cel3ds \;
find ../../src/cel3ds/ -name "*.h" -exec cp {} src/cel3ds \;

find ../../src/celengine/ -name "*.cpp" -exec cp {} src/celengine \;
find ../../src/celengine/ -name "*.h" -exec cp {} src/celengine \;

find ../../src/celestia/ -name "*.cpp" -exec cp {} src/celestia \;
find ../../src/celestia/ -name "*.h" -exec cp {} src/celestia \;

find ../../src/celmath/ -name "*.cpp" -exec cp {} src/celmath \;
find ../../src/celmath/ -name "*.h" -exec cp {} src/celmath \;

find ../../src/celtxf/ -name "*.cpp" -exec cp {} src/celtxf \;
find ../../src/celtxf/ -name "*.h" -exec cp {} src/celtxf \;

find ../../src/celutil/ -name "*.cpp" -exec cp {} src/celutil \;
find ../../src/celutil/ -name "*.h" -exec cp {} src/celutil \;

find ../../src/ -depth 1 -name "*.cpp" -exec cp {} src/celutil \;
find ../../src/ -depth 1 -name "*.h" -exec cp {} src/celutil \;

cp -rf ../../src/tools src/ 

# write diff file before updating
cd src
your_diff="r${svn_rev}-diff.diff"
svn diff > ../../${your_diff}

echo $SEPAR
echo " Your diff from last $svn_rev revision are stored into $your_diff"
echo $SEPAR
echo " Updating and merging..."

# update 
update_file="svn-update.log"
script ../../$update_file svn up
# print info
echo $SEPAR
updated=`cat ../../$update_file| grep "^U " | wc -l`
conflicts=`cat ../../$update_file| grep "^C "| wc -l`
merged=`cat ../../$update_file| grep "^G " | wc -l`
added=`cat ../../$update_file| grep "^A " | wc -l`
deleted=`cat ../../$update_file| grep "^D " | wc -l`
echo " New files: $added"
echo " Deleted:   $deleted"
echo " Updated:   $updated"
echo " Merged:    $merged"
echo " Conflicts: $conflicts"
echo $SEPAR
echo " * See $update_file for conflicts in work/ dir."
echo " * Run ./commit-svn.sh to apply updates."
new_rev=`cat ../../$update_file | grep "Updated to revision" | awk '{print $4}'| sed 's/\.//`
echo " * You can use command 'cd work/src/ && svn diff -r${svn_rev}:${new_rev}' to take celestia's diff from last revision"
cd ../../









