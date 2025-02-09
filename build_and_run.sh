#!/bin/bash

set -e

mkdir -p build
cd build

cmake ..
make

./r-type_server 8080
