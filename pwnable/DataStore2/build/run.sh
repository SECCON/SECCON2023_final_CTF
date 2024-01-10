#!/bin/sh

cd $(dirname $0)
exec timeout -sKILL 30 ./chall
