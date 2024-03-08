dep
======

General purpose dependency manager, with a slight focus on static C libraries

Summary
-------

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

By default, no default repositories are enabled, so it's advisable to run the
following to enable the official repository:

```sh
dep repository add finwo https://github.com/finwo/dep-repository/archive/refs/heads/main.tar.gz
```

Usage
-----

#### Update repositories

dep keeps local cache of the repositories you have enabled. To update this
cache, run the following command

```sh
dep repository update
```

#### Adding a dependency to your project

To add a package, you can call the following command to install a specific
version of the package:

```sh
dep add package/identifier@version
```

If you have package.channel set in your project's package.ini, you can also
leave the version from the command to automatically select the version you've
set there.

```ini
[package]
channel=edge
```

```sh
dep add package/identifier
```

For example, if you'd want to install the [finwo/palloc][palloc] package, you
could use the following comamnd:

```sh
dep add finwo/palloc@edge # with version specifier
dep add finwo/palloc      # without version specifier
```

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

[palloc]: https://github.com/finwo/palloc.c
