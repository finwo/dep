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

function install {
  case "$(command -v apt apt-get apk xbps-install | head -1)" in
    apt)
      apt install "$1" -y -qq
      ;;
    apt-get)
      apt-get install "$1" -y -qq
      ;;
    apk)
      apk add "$1"
      ;;
    xbps-install)
      xbps-install "$1"
      ;;
    *)
      echo "No supported package manager detected"
      echo "Please install '$1' to run this script"
      exit 1
      ;;
  esac
}

# Ensure our dependencies
command -v curl &>/dev/null || install curl

# Default install directory
mkdir -p /usr/local/bin

# Main script
curl -L# https://raw.githubusercontent.com/cdeps/dep/master/dep > /usr/local/bin/dep
chmod +x /usr/local/bin/dep

# Repository manager
curl -L# https://raw.githubusercontent.com/cdeps/dep/master/dep-repo > /usr/local/bin/dep-repo
chmod +x /usr/local/bin/dep-repo
