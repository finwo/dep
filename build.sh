#!/usr/bin/env bash

# Load pre-processor
[ -f bashpp ] || {
  curl https://raw.githubusercontent.com/iwonbigbro/bashpp/master/bin/bashpp > bashpp
  chmod +x bashpp
}

# Refresh dist dir
rm -rf dist
mkdir dist

# Build outputs
./bashpp src/dep      > dist/dep
./bashpp src/dep-repo > dist/dep-repo
chmod +x dist/dep
chmod +x dist/dep-repo
