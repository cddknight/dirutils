#!/bin/bash

START_PWD=${PWD}

function doBigText
{
	TEXTCMD=$(which bigWord 2> /dev/null)
	if [ "${TEXTCMD}" != "" ] && [ "${1}" != "" ]
	then
		${TEXTCMD} -c 14 "$1"
	else
		echo $1
	fi
}

doBigText "DirUtils Packages"

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

