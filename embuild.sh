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

emmake make -j 16

# to run, do
#  python -m SimpleHTTPServer 8080
#  x-www-browser http://localhost/ogm.html
