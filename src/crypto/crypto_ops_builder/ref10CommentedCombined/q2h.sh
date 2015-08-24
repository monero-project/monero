#!/bin/sh
sed 's/^#.*//' \
| qhasm-generic \
| sed 's_//\(.*\)$_/*\1 */_'
