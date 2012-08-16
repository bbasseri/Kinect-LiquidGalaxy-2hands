#!/bin/bash
rm -f /tmp/fakenav
rm -f ./fakenav
make
mkfifo /tmp/fakenav
./fakenav
