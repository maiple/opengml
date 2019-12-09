rm -r release/ > /dev/null

cmake -DRELEASE=ON .
make -j 8

if [ $? == 0 ]; then
    mkdir release/
    cp ogm.exe release/
    cp *.dll > release/
    cp README.md > release
fi
