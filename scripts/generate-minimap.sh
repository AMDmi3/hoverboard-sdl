#!/bin/sh

for y in `seq -- -1112 -1069`; do
	echo "Processing line $y..."
	for x in `seq 928 1107`; do
		if [ -e "$x/$y.png" ]; then
			echo "$x/$y.png"
		else
			echo "null:"
		fi
	done | xargs -J % montage % -depth 8 -tile 180x -crop 512x512+0+0 -geometry 8x8+0+0 line_$y.png &
done

wait

for y in `seq -- -1112 -1069`; do
	echo "line_$y.png"
done | xargs -J % montage % -tile 1x -geometry +0+0 minimap.png

rm line_*.png

optipng -o99 minimap.png
