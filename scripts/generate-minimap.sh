#!/bin/sh
#
# Run this script from data/ directory to generate minimap
#

set -e

for y in `seq -- -1112 -1069`; do
	echo "===> Merging line $y..."
	for x in `seq 928 1107`; do
		if [ -e "$x/$y.png" ]; then
			echo "$x/$y.png"
		else
			echo "null:"
		fi
	done | xargs -J % montage % -depth 8 -tile 180x -crop 512x512+0+0 -geometry 8x8+0+0 line_$y.png
done

echo "===> Merging lines into image..."
for y in `seq -- -1112 -1069`; do
	echo "line_$y.png"
done | xargs -J % montage % -tile 1x -geometry +0+0 minimap.base.png

rm line_*.png

echo "===> Inverting colors..."
convert minimap.base.png -negate minimap.inverted.png
rm minimap.base.png

echo "===> Converting into alpha mask..."
convert minimap.inverted.png -background Gray -alpha shape minimap.png
rm minimap.inverted.png

echo "===> Optimizing..."
optipng -o99 -strip all minimap.png
