#!/bin/sh

set -eu

NOCT=${NOCT:-../../build-static/noct}

echo 'Target-neutral accelerator loop analysis tests:'

for opt in 0 1 2 3; do
	: > analysis.out
	for range in 1:7 9:14 16:22 24:29 31:36 38:46 \
		     48:55 57:62 64:70 72:78 80:86; do
		start=${range%:*}
		end=${range#*:}
		awk -v start="$start" -v end="$end" \
			'{ if (NR >= start && NR <= end) print; else print "" } \
			 END { print "func main() {"; print "}" }' \
			accel/analysis.noct > analysis-case.noct
		LC_ALL=C "$NOCT" --disable-accel --accel-info -j0 -O"$opt" \
			analysis-case.noct > analysis.stdout 2> analysis.stderr || true
		grep '^accel-analysis ' analysis.stderr >> analysis.out
	done
	diff accel/analysis.noct.out analysis.out
done

rm -f analysis-case.noct analysis.stdout analysis.stderr analysis.out
echo 'PASS'
