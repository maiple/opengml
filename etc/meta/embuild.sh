#!/bin/bash
# This builds the project for use with emscripten
# emscripten support is still experimental

emcc --version
if [ $? -ne 0 ]; then
  echo "emscripten not installed."
  echo "See https://emscripten.org/docs/getting_started/Tutorial.html"
  exit 1
fi

export EMSCRIPTEN=1

emconfigure cmake .

if [ $? -ne 0 ]; then
    exit 1
fi

emmake make -j 16

if [ $? -ne 0 ]; then
    exit 1
fi

# to run, do ./emlaunch.sh

echo "to run, use:"
echo "python -m SimpleHTTPServer 8080"
echo "then connect to http://localhost:8080/ogm.html with a web browser."
