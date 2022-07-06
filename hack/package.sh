#!/bin/bash
#
# A script used to package dnscrypt-wrapper. It depends on docker.
#

ROOT=$(unset CDPATH && cd $(dirname "${BASH_SOURCE[0]}")/.. && pwd)
cd $ROOT

docker build -t dns