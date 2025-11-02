curdir="$(dirname "$0")"

rm -rf build/*
cmake -S "$curdir"/.. -B "$curdir"/../build "$1"
cmake --build "$curdir"/../build
./"$curdir"/../build/spectral_main

