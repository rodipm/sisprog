#!/bin/bash

#########################
# Usage:
# watch.sh  <file>
#########################
inotifywait -m -e moved_to,create . | 
	while read path event file; do
		if [ "$file" == "main.cpp" ] || [ "$file" == "Assembler.cpp" ] || [ "$file" == "VM.cpp" ] || [ "$file" == "loader.asm" ] || [ "$file" == "test.asm" ] ; then
			clear
			#g++ -o ${1:0:(-4)} $1
			g++ -o main main.cpp Assembler.cpp VM.cpp
			./main
		fi
	done	
