# Origin: https://stackoverflow.com/a/25394801

windows() { [[ -n "$WINDIR" ]]; }

# Cross-platform symlink function. With one parameter, it will check
# whether the parameter is a symlink. With two parameters, it will create
# a symlink to a file or directory, with syntax: link $linkname $target
lnk() {
    if [[ -z "$2" ]]; then
        # Link-checking mode.
        if windows; then
            fsutil reparsepoint query "$1" > /dev/null
        else
            [[ -h "$1" ]]
        fi
    else
        # Link-creation mode.
        if windows; then
            # Windows needs to be told if it's a directory or not. Infer that.
            # Also: note that we convert `/` to `\`. In this case it's necessary.
            if [[ -d "$2" ]]; then
                echo 'cmd <<< "mklink /D \"$1\" \"${2//\//\\}\"" > /dev/null'
                cmd <<< "mklink /D \"$1\" \"${2//\//\\}\"" > /dev/null
            else
                echo 'cmd <<< "mklink \"$1\" \"${2//\//\\}\"" > /dev/null'
                cmd <<< "mklink \"$1\" \"${2//\//\\}\"" > /dev/null
            fi
        else
            echo ln -fs "$2" "$1"
            ln -fs "$2" "$1"
        fi
    fi
}
