#!/bin/bash

#########################
# Usage:
# watch.sh  <file>
#########################
inotifywait -m -e moved_to,create . | 
	while read path event file; do
		if [ "$file" == $1 ]; then
			clear
			g++ -o ${1:0:(-4)} $1 main.cpp
			./${1:0:(-4)}
		fi
	done	
