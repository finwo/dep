# #ifndef __OSTYPE_SH__
# #define __OSTYPE_SH__

function ostype {
  case "$OSTYPE" in
    darwin*) echo "osx" ;;
    linux*)  echo "lin" ;;
    bsd*)    echo "bsd" ;;
    msys*)   echo "win" ;;
    cygwin*) echo "win" ;;
    *)       echo "unknown" ;;
  esac
}

# __OSTYPE_SH__
# #endif
