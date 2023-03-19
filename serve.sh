#!/bin/bash

if ! [[ "$_SERVE_STATUS" = "run" ]]; then
	export _SERVE_STATUS=run
	port=$1
	[[ "$port" = "" ]] && port=8080
	echo "Running server on http://localhost:$port/"
	while true; do
		nc -l -p $port -e "$0" || exit 0
	done

	echo "Please fix you'r /bin/true"
	exit 1
fi

read ctx
path="$(echo "$ctx" | sed 's/GET \/\([^ \?\#]*\)[^ ]* HTTP\/1\.1/\1/' | sed 's/\r//g')"

[[ "$path" == *".."* ]] && path=404
[[ "$path" = "" ]] && path=index.html
path="out/$path"
path="$(echo "$path" | sed 's/\.\(txt\)$/.html/')"
[[ "${path##*.}" = "$path" ]] && path="$path.html"

if ! ./compile.sh > /tmp/serve-log.txt 2>&1; then
	status="500 INTERNAL SERVER ERROR"
	path="/tmp/serve-log.txt"
elif ! test -e "$path" || [[ "$path" = "out/404.html" ]]; then
	status="404 NOT FOUND"
	path="out/404.html"
else
	status="200 OK"
fi

len=$(wc -c "$path")

case "$path" in
	*.json)
		type="application/json"
		;;
	*.html)
		type="text/html"
		;;
	*.css)
		type="text/css"
		;;
	*)
		type="text/plain"
		;;
esac

echo "HTTP/1.1 $status"
echo "Content-Type: $type"
echo "Content-Length: ${len%% *}"
echo
cat "$path"

exit 0

