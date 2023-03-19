#!/bin/bash

if [[ "$1" = "clean" ]]; then
	echo "Cleanup"
	rm -rf out/ bin/
	exit 0
fi

[[ "$1" = "--release" ]] && is_debug=false || is_debug=true
debug() {
	[ $is_debug = true ]
	return $?
}

mkdir -p bin/
if which tcc >/dev/null 2>&1; then
	tcc -o bin/fmf fmf.c || exit 1
else
	cc -o bin/fmt fmt.c || exit 1
fi
fmf=../bin/fmf

rm -rf out/
mkdir -p out/
cd src/
for file in $(find -iname '*.fmf'); do
	name="${file%%.fmf}"
	bname="$(basename "$name")"
	mkdir -p ../out/$(dirname "$name")
	sed ../parts/begin.html \
		-e 's/${name}/'"$bname"'/g' > ../out/$name.html
	$fmf "$file" >> ../out/$name.html || exit 2
	sed ../parts/end.html \
		-e 's/${name}/'"$bname"'/g' >> ../out/$name.html
done

