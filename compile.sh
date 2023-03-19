#!/bin/bash

[[ "$1" = "--release" ]] && is_debug=false || is_debug=true
debug() {
	[ $is_debug = true ]
	return $?
}

if debug; then
	tcc -o fmf fmf.c
	tccver="$(tcc --version)"
else
	clang -O3 -o fmf fmf.c
fi

rm -rf out/
mkdir -p out/
for file in *.fmf; do
	name="${file%%.fmf}"
	sed begin.html \
		-e 's/${name}/'"$name"'/g' > out/$name.html
	./fmf "$file" >> out/$name.html
	sed end.html \
		-e 's/${name}/'"$name"'/g' >> out/$name.html
	if debug; then
		echo "<!-- generated on $(date --utc), $tccver -->" >> out/$name.html
	fi
done

