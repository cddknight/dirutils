#!/bin/bash

START_PWD=${PWD}

for DIR in lib utils
do
	if [ -d ${DIR} ]
	then
		cd ${DIR}
		if [ -e builddeb.sh ]
		then
			./builddeb.sh
		fi
		cd ${START_PWD}
	fi
done

