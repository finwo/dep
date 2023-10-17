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
  r(epo(sitory))  Repository management

Help topics:
  global          This help text
  add             More detailed explanation on the add command
  install         More detailed explanation on the install command
  repository      More detailed explanation on the repository command
```

Installation
------------

To install dep, simply download [dist/dep](dist/dep) and place it in your
`/usr/local/bin` directory or anywhere else that's included in your `$PATH`.

Building
--------

Building this dependency manager requires
[preprocess](https://pypi.org/project/preprocess/) which you can install by
running `pip install preprocess`.

After fetching the preprocess dependency, you can build & install dep by running
the following commands:

```sh
make
sudo make install
```

To install the binary in a location other than `/usr/local/bin`, pass the
`DESTDIR` definition to the `make install` command (default: `/usr/local`).

License
-------

This project falls under the [MIT license](LICENSE)
