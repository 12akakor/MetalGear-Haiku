#!/bin/sh
# Downloads zexdoc.com and zexall.com (Frank Cringle, GPL), the standard Z80
# instruction tests, from the z80emu project.
cd "$(dirname "$0")"
for f in zexdoc.com zexall.com; do
	curl -sSLo $f https://raw.githubusercontent.com/anotherlin/z80emu/master/testfiles/$f
done
