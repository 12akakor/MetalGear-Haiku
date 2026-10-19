#!/bin/sh
# Runs routine_diff in several situations: the title and demo, random play
# with warps, and with all weapons and items. Usage: run.sh <rom> [names...]
rom=$1
shift
status=0
for args in "8000" "30000 -play -warp 300 -seed 1" "40000 -play -gear -warp 300 -seed 2" \
		"40000 -play -gear -warp 200 -seed 3"; do
	./routine_diff "$rom" $args "$@" > /tmp/routine_diff.$$ || status=1
	grep "FAIL" /tmp/routine_diff.$$ | head -20
	tail -1 /tmp/routine_diff.$$
done
rm -f /tmp/routine_diff.$$
exit $status
