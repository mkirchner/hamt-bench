#!/bin/sh

usage() {
    echo "$0 <group-tag> <run-tag> [hamt glib avl rb hsearch]"
}

# make sure we have reasonable command line parameters
if [ $# -lt 3 ]; then
    usage
    exit -1
fi

# make sure the DB exists
if [ ! -f "db/db.sqlite" ]; then
    sqlite db/db.sqlite < db/benchmark.sql
fi

GROUP_TAG=$1
RUN_TAG=$2
shift 
shift
for name in $@; do
    case $name in
        hamt)
            GIT_BRANCH=`(cd lib/hamt && git branch --show-current)`
            GIT_BRANCH=`echo $GIT_BRANCH | sed -e 's#/#-#g'`
            GIT_COMMIT=`(cd lib/hamt && git describe --always)`
            echo "hamt-malloc-murmur"
            build/bench-hamt -a malloc -H murmur3 -w 2 -q | sed -u -e "s/^/$GROUP_TAG,hamt-malloc-murmur-$RUN_TAG,$GIT_COMMIT,/" >> db/import.$$
            echo "hamt-malloc-uh"
            build/bench-hamt -a malloc -H uh -w 2  -q | sed -u -e "s/^/$GROUP_TAG,hamt-malloc-uh-$RUN_TAG,$GIT_COMMIT,/" >> db/import.$$
            echo "hamt-dlmalloc-murmur"
            build/bench-hamt -a dlmalloc -H murmur3 -w 2  -q | sed -u -e "s/^/$GROUP_TAG,hamt-dlmalloc-murmur-$RUN_TAG,$GIT_COMMIT,/" >> db/import.$$
            echo "hamt-dlmalloc-uh"
            build/bench-hamt -a dlmalloc -H uh -w 2  -q | sed -u -e "s/^/$GROUP_TAG,hamt-dlmalloc-uh-$RUN_TAG,$GIT_COMMIT,/" >> db/import.$$
            echo "hamt-bdwgc-murmur"
            build/bench-hamt -a bdwgc -H murmur3 -w 2 -q | sed -u -e "s/^/$GROUP_TAG,hamt-bdwgc-murmur-$RUN_TAG,$GIT_COMMIT,/" >> db/import.$$
            echo "hamt-bdwgc-uh"
            build/bench-hamt -a bdwgc -H uh -w 2  -q | sed -u -e "s/^/$GROUP_TAG,hamt-bdwgc-uh-$RUN_TAG,$GIT_COMMIT,/" >> db/import.$$
            ;;
        glib)
            echo "glib-hashtable"
            build/bench-glib | sed -u -e "s/^/"$GROUP_TAG","glib2","",/" >> db/import.$$
            ;;
        avl)
            echo "avl"
            build/bench-avl | sed -u -e "s/^/"$GROUP_TAG","avl","",/" >> db/import.$$
            ;;
        rb)
            echo "rb"
            build/bench-rb | sed -u -e "s/^/"$GROUP_TAG","rb","",/" >> db/import.$$
            ;;
        hsearch)
            echo "hsearch"
            build/bench-hsearch | sed -u -e "s/^/"$GROUP_TAG","hsearch","",/" >> db/import.$$
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
