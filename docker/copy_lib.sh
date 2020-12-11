# copies the library at the given line of ldd output the output directory.

out="$1"
shift 1

lib="$1"
srclib="$3"

# skip invalid args
if [[ "$lib" = "/*" ]];
then
    exit 0
fi

if ! [[ "$srclib" = "/*" ]];
then
    exit 0
fi

cp "$srclib" "$out/$lib"
echo "$lib is $(stat '$srclib') bytes"