#include "ini.sh"

# Filled by bashpp
NAME=__NAME

function main {

  # Commands:
  #   install


  echo "Hello World"
}

if [ $(basename $0) == "${NAME}" ]; then
  main
fi
