#!/bin/bash

#
# run profiling
#
UUID=`CPUPROFILE_FREQUENCY=10000 build/profile-hamt`

#
# create reports
#
INSERT_TRANSIENT="hamt-insert-transient-${UUID}"
INSERT_PERSISTENT="hamt-insert-persistent-${UUID}"
pprof --pdf build/profile-hamt "${INSERT_TRANSIENT}.prof" > "${INSERT_TRANSIENT}.pdf"
pprof --pdf build/profile-hamt "${INSERT_PERSISTENT}.prof" > "${INSERT_PERSISTENT}.pdf"
open "${INSERT_TRANSIENT}.pdf" "${INSERT_PERSISTENT}.pdf"
