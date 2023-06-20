#!/bin/bash

START_PWD=${PWD}

for DIR in libdircmd utils
do
	if [ -d ${DIR} ]
	then
		lineDraw
		echo Building: $DIR package
		lineDraw
		cd ${DIR}
		if [ -e buildpkg.sh ]
		then
			./buildpkg.sh
		fi
		cd ${START_PWD}
	fi
done

