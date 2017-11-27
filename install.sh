#!/usr/bin/env bash

# Verify root privileges
if [[ $EUID -ne 0 ]]; then
  echo "This script must be run as root" >&2
  exit 1
fi

# Print "logo"
echo " ,_,"
echo " )v(  DEP general-purpose"
echo " \\_/      dependency manager"
echo "==\"=="

# Check if curl is present
command -v curl &>/dev/null || {

  # Check if we can install it
  command -v apt-get &>/dev/null || {
    echo "cUrl is missing and we're not able to install it" >&2
    exit 1
  }

  echo "Installing cUrl"
  apt-get install -y -qq curl || {
    echo "Unable to install cUrl"
  }

}

# Default install directory
mkdir -p /usr/local/bin

# Main script
curl -L# https://raw.githubusercontent.com/cdeps/dep/master/dep > /usr/local/bin/dep
chmod +x /usr/local/bin/dep

# Repository manager
curl -L# https://raw.githubusercontent.com/cdeps/dep/master/dep-repo > /usr/local/bin/dep-repo
chmod +x /usr/local/bin/dep-repo
