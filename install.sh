#!/usr/bin/env bash

# Verify root privileges
if [[ $EUID -ne 0 ]]; then
  echo "This script must be run as root" >&2
  exit 1
fi

# Print banner
cat <<EOF
  ,_,   DEP
  )v(
  \_/   general-purpose
 =="==  dependency manager

EOF

# Install repo binary
function bin {
  [ -f "./$1" ] && {
    echo "Copying $1"
    cp "./$1" "/usr/local/bin/$1"
  } || curl -L# "https://raw.githubusercontent.com/cdeps/dep/master/dist/$1" > "/usr/local/bin/$1"
  chmod +x "/usr/local/bin/$1"
}

# Ensure our dependencies
command -v curl &>/dev/null || {
  echo "cUrl needs to be installed to use dep"
}

# Default install directory
mkdir -p /usr/local/bin

# Install our binaries
bin dep
bin dep-repo
