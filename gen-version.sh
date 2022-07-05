#!/bin/sh
# Generate version automatically.

version_file=version.h

if test -d .git; then
    version=$(git describe --always --match "v[0-9]*" | sed -r -e 's/-([^-]+)$/.\1/g' | sed -e 's/^v//g')
    if test -f "$version_file