#!/bin/sh

GITCOMMIT=`(cd ../hamt && git describe --always)`
echo "hamt"
build/bench-hamt | sed -u -e "s/^/"libhamt",$GITCOMMIT,/" > db/import.$$
echo "glib-hashtable"
build/bench-glib | sed -u -e "s/^/"glib2","",/" >> db/import.$$
echo "avl"
build/bench-avl | sed -u -e "s/^/"avl","",/" >> db/import.$$
echo "rb"
build/bench-rb | sed -u -e "s/^/"rb","",/" >> db/import.$$
# echo "hsearch"
# build/bench-hsearch | sed -u -e "s/^/"hsearch","",/" >> db/import.$$

{
cat << EOF
.mode csv
.import db/import.$$ numbers
EOF
} | sqlite3 db/db.sqlite
