#!/bin/bash

[[ "$1" = "--release" ]] && is_debug=false || is_debug=true
debug() {
	[ $is_debug = true ]
	return $?
}

if debug && which tcc >/dev/null 2>&1; then
	tcc -o fmf fmf.c || exit 1
	tccver="$(tcc --version)"
fi

rm -rf out/
mkdir -p out/
for file in *.fmf; do
	name="${file%%.fmf}"
	sed begin.html \
		-e 's/${name}/'"$name"'/g' > out/$name.html
	./fmf "$file" >> out/$name.html || exit 2
	sed end.html \
		-e 's/${name}/'"$name"'/g' >> out/$name.html
done

