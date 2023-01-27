# #ifndef __INI_SH__
# #define __INI_SH__

# #include "shopt.sh"

# Arguments:
#   $0 <fn_keyHandler> <str_filename> [section[.key]]
function ini_foreach {

  # No file = no data
  inifile="${2}"
  if [[ ! -f "$inifile" ]]; then
    exit 1
  fi

  # Process the file line-by-line
  SECTION=
  while read line; do

    # Remove surrounding whitespace
    line=${line##*( )} # From the beginning
    line=${line%%*( )} # From the end

    # Remove comments and empty lines
    if [[ "${line:0:1}" == '#' ]] || [[ "${line:0:1}" == ';' ]] || [[ "${#line}" == 0 ]]; then
      continue
    fi

    # Handle section markers
    if [[ "${line:0:1}" == "[" ]]; then
      SECTION=$(echo $line | sed -e 's/\[\(.*\)\]/\1/')
      SECTION=${SECTION##*( )}
      SECTION=${SECTION%%*( )}
      SECTION="${SECTION}."
      continue
    fi

    # Output found variable
    NAME=${line%%=*}
    NAME=${NAME%%*( )}
    VALUE=${line#*=}
    VALUE=${VALUE##*( )}

    # Output searched or all
    if [[ -z "${3}" ]]; then
      $1 "$SECTION" "$NAME" "${VALUE}"
    elif [[ "${SECTION}" == "${3}" ]] || [[ "${SECTION}${NAME}" == "${3}" ]]; then
      $1 "$SECTION" "$NAME" "${VALUE}"
    fi

  done < "${inifile}"

}

function ini_output_full {
  echo "$1$2=$3"
}
function ini_output_section {
  echo "$2=$3"
}
function ini_output_value {
  echo "$3"
}

# Allow this file to be called stand-alone
# ini.sh <filename> [section[.key]] [sectionmode]
if [ $(basename $0) == "ini.sh" ]; then
  fullMode=full
  sectionMode=value
  if [[ ! -z "${3}" ]]; then
    fullMode=${3}
    sectionMode=${3}
  fi
  if [[ -z "${2}" ]]; then
    ini_foreach ini_output_${fullMode} "$@"
  else
    ini_foreach ini_output_${sectionMode} "$@"
  fi
fi

# __INI_SH__
# #endif
