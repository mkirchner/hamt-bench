#! /bin/sh

GITCOMMIT=`(cd ../hamt && git describe --always)`
build/bench-hamt | sed -u -e "s/^/"libhamt",$GITCOMMIT,/" > db/import.$$

build/bench-glib | sed -u -e "s/^/"glib2","",/" >> db/import.$$

build/bench-hsearch | sed -u -e "s/^/"hsearch","",/" >> db/import.$$

{
cat << EOF
.mode csv
.import db/import.$$ numbers
EOF
} | sqlite3 db/db.sqlite
