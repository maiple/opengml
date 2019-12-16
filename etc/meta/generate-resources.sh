# return to root directory
cd `git rev-parse --show-toplevel`

cd etc/resources/
dst="../../src/resources"
header="${dst}/resources.h"
echo "#pragma once" > $header
echo "" >> $header
for filename in *; do
    name="${filename//./_}"
    echo "embedding \"${filename}\" as \"${name}\""
    echo "extern unsigned char* ${name};" >> $header
    echo "extern unsigned int ${name}_len;" >> $header
    xxd -i "${filename}" > "${dst}/${filename}.c"
done
