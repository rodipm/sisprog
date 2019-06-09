#!/bin/bash

# Auto Refresher
# auto_refresh.sh path "command"

inotifywait -q -e close_write,moved_to -m . |
while read -r directory events filename; do
	if [ "$filename" = "$1" ]; then
		clear
		$2
	fi
done
