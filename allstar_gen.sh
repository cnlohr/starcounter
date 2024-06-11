#!/bin/bash

gh repo list $1 -L 4000 --json name --jq '(map(keys) | add | unique) as $cols | map(. as $row | $cols | map($row[.])) as $rows | $cols, $rows[] | @tsv' | tail +2 > repolist.txt
#curl https://api.github.com/repos/cnlohr/ch32v003fun/stargazers -H 'Accept: application/vnd.github.v3.star+json'
set -x

echo "Got repo list."

rm -rf allstarlist.txt

while read r; do
	echo $r
	P=1
	T=1
	while [ $T -ne 0 ]
	do
		gh api "repos/$1/$r/stargazers?per_page=100&page=$P" -H 'Accept: application/vnd.github.v3.star+json' | jq -r '.[] | [.starred_at] | @csv' > temp.txt
		T=$(wc temp.txt  -l | cut -d' ' -f1)
		echo $T

		while read t; do
			temp="${t%\"}"
			temp="${temp#\"}"
			echo -ne "$r," >> allstarlist.txt
			date -d $temp "+%s" >> allstarlist.txt
		done < temp.txt

		echo $P
		P=$((P+1))
	done
done < repolist.txt



