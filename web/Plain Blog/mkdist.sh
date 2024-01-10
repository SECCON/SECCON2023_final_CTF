#!/bin/bash
set -eu

if [ -d files ]; then
    rm -r files
fi

mkdir files
cp -r build files/plain-blog
find files/plain-blog -maxdepth 3 -type d -name __pycache__ | xargs --no-run-if-empty rm -r

echo "SECCON{dummy}" > files/plain-blog/app/flag.txt
echo -n "PASSWORD_DUMMY" > files/plain-blog/app/password.txt
