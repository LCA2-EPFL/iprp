#!/bin/sh

rm -rf files/
mkdir files

iptables -t mangle -F

bin/icd "$@"
