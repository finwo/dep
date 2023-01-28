dep
======

General purpose dependency manager, with a slight focus on static C libraries

Usage
-----

```
Usage: dep [global options] <command> [options] [-- ...args]

Global options:
  n/a

Commands:
  a(dd)           Add a new dependency to the project
  i(nstall)       Install all the project's dependencies
  h(elp) [topic]  Show this help or the top-level info about a command

Help topics:
  global          This help text
  add             More detailed explanation on the add command
  install         More detailed explanation on the install command
```

Installation
------------

To install dep, simply download [dist/dep](dist/dep) and place it in your
`/usr/local/bin` directory or anywhere else that's included in your `$PATH`.

If you want to build it from source, clone the repository and run the following
commands:

```sh
make
sudo make install
```

To install the binary in a location other that `/usr/local/bin`, pass the
`DESTDIR` definition to the `make install` command (default: `/usr/local`).

License
-------

This project falls under the [MIT license](LICENSE)
