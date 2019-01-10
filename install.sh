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

# Add system package
function system {
  command -v apt          &>/dev/null && { apt     install "$1" -y -qq; return; }
  command -v apt-get      &>/dev/null && { apt-get install "$1" -y -qq; return; }
  command -v apk          &>/dev/null && { apk     add     "$1"       ; return; }
  command -v xbps-install &>/dev/null && { xbps-install    "$1"       ; return; }
  echo "No supported package manager detected"
  echo "Please install '$1' to run this script"
  exit 1
}

# Install repo binary
function bin {
  [ -f "./$1" ] && {
    echo "Copying $1"
    cp "./$1" "/usr/local/bin/$1"
  } || curl -L# "https://raw.githubusercontent.com/cdeps/dep/master/$1" > "/usr/local/bin/$1"
  chmod +x "/usr/local/bin/$1"
}

# Ensure our dependencies
command -v curl &>/dev/null || system curl

# Default install directory
mkdir -p /usr/local/bin

# Install our binaries
bin dep
bin dep-repo
