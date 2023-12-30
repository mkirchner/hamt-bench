#!/bin/sh

usage() {
    echo "$0 <tag> [hamt glib avl rb hsearch]"
}

# make sure we have reasonable command line parameters
if [ $# -lt 2 ]; then
    usage
    exit -1
fi

# make sure the DB exists
if [ ! -f "db/db.sqlite" ]; then
    sqlite db/db.sqlite < db/benchmark.sql
fi

TAG=$1
shift 
for name in $@; do
    case $name in
        hamt)
            GIT_BRANCH=`(cd ../hamt && git branch --show-current)`
            GIT_COMMIT=`(cd ../hamt && git describe --always)`
            echo "hamt"
            build/bench-hamt | sed -u -e "s/^/\"$TAG\",\"libhamt:$GIT_BRANCH\",$GIT_COMMIT,/" > db/import.$$
            ;;
        glib)
            echo "glib-hashtable"
            build/bench-glib | sed -u -e "s/^/"$TAG","glib2","",/" >> db/import.$$
            ;;
        avl)
            echo "avl"
            build/bench-avl | sed -u -e "s/^/"$TAG","avl","",/" >> db/import.$$
            ;;
        rb)
            echo "rb"
            build/bench-rb | sed -u -e "s/^/"$TAG","rb","",/" >> db/import.$$
            ;;
        hsearch)
            echo "hsearch"
            build/bench-hsearch | sed -u -e "s/^/"$TAG","hsearch","",/" >> db/import.$$
            ;;
        *)
            usage
            exit -1
    esac
done

{
cat << EOF
.mode csv
.import db/import.$$ numbers
EOF
} | sqlite3 db/db.sqlite
