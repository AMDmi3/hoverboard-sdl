#!/bin/sh
#
# Run this script from data/ directory to extract png icon
#

set -e

convert xkcd.ico xkcd.png
mv xkcd-1.png xkcd.png
rm -f xkcd-0.png
optipng -o99 -strip all xkcd.png
