#!/usr/bin/env bash

# Verify root privileges
if [[ $EUID -ne 0 ]]; then
  echo "This script must be run as root" >&2
  exit 1
fi

# Print banner
cat <<EOF
 ,_,
 )v( DEP general-purpose
 \_/     dependency manager
=="==
EOF

# Add system package
function system {
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

# Install repo file
function install {
  [ -f "./$1" ] && {
    echo "Copying $1"
    cp "./$1" "/usr/local/bin/$1"
  } || {
    curl -L# "https://raw.githubusercontent.com/cdeps/dep/master/$1" > "/usr/local/bin/$1"
  }
  chmod +x "/usr/local/bin/$1"
}

# Ensure our dependencies
command -v curl &>/dev/null || system curl

# Default install directory
mkdir -p /usr/local/bin

# Install our binaries
install dep
install dep-repo
