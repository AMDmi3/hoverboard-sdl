#!/bin/sh
#
# Run this script from data/ directory to download tiles
#

for x in `seq 928 1107`; do
	for y in `seq -- -1150 -1000`; do
		echo -n "[$x,$y]: "
		if [ -e "$x/$y.png" ]; then
			echo "already downloaded"
			continue
		fi

		rm -f current.png

		if wget -q "http://xkcd.com/1608/$x:$y+s.png"; then
			echo "downloaded"
			mkdir -p $x
			mv "$x:$y+s.png" $x/$y.png
		else
			echo "doesn't exist"
		fi
	done
done
