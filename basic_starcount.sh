#!/bin/bash

gh repo list $1 -L 4000 --json name,stargazerCount --jq '(map(keys) | add | unique) as $cols | map(. as $row | $cols | map($row[.])) as $rows | $cols, $rows[] | @tsv' | tail +2 | cut -d$'\t' -f2 | paste -sd+ | bc

