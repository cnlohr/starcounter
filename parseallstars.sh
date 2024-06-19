#!/bin/bash

cat $1 | sed -e "s/,/ /g" | sort -k2 > temp.txt

ltime=0
stars=1

echo "" > starsbydate.txt

while read l
do
	repo=$(echo $l | cut -f1 -d' ')
	ctime=$(echo $l | cut -f2 -d' ')
	if (test $ltime -lt 1)
	then
		ltime=$(($ctime / 86400 * 86400 + 86400))
	fi
	stars=$(($stars + 1))
	while (test $ctime -gt $ltime)
	do
		readabledate=$(date -d @$ltime -u +'%Y-%m-%d')
		echo $readabledate,$stars | tee -a starsbydate.txt
		#echo $ltime $stars
		ltime=$(($ltime + 86400))
	done
done < temp.txt

echo written to starsbydate.txt
