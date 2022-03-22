#! /bin/sh

GITCOMMIT=`(cd ../hamt && git describe --always)`
build/bench-hamt | sed -u -e "s/^/$GITCOMMIT,/" > db/import.$$

{
cat << EOF
.mode csv
.import db/import.$$ numbers
EOF
} | sqlite3 db/db.sqlite
