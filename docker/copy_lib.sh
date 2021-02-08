# copies the library at the given line of ldd output the output directory.

out="$1"
shift 1

# https://stackoverflow.com/a/3352015
trim() {
    local var="$*"
    # remove leading whitespace characters
    var="${var#"${var%%[![:space:]]*}"}"
    # remove trailing whitespace characters
    var="${var%"${var##*[![:space:]]}"}"   
    printf '%s' "$var"
}

# trim
in=`trim "$1"`
lib=`echo "$in" | cut -d' ' -f1`
srclib=`echo "$in" | cut -d' ' -f3`

# recursively resolve symlinks in srclib
srclib=`python -c "import os; print(os.path.realpath(\"$srclib\"))"`

# skip invalid args
if [[ "$lib" = /* ]];
then
    exit 0
fi

if ! [[ "$srclib" = /* ]];
then
    exit 0
fi

echo "copying $srclib to $out/$lib"

cp "$srclib" "$out/$lib"
sstat=`stat -c "%s" "$srclib"`
echo "$lib is $sstat bytes"