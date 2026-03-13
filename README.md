dep
===

General purpose dependency manager for embedded C libraries, written in C.

Summary
-------

```
Usage: dep [global options] <command> [command options]

Global options:
  n/a

Commands:
  a(dd)           Add a new dependency to the project
  i(nstall)       Install all the project's dependencies
  i(nit)          Initialize a new project with a .dep file
  l(icense)       Show license information
  r(epo(sitory))  Repository management
  h(elp) [topic]  Show this help or the top-level info about a command

Help topics:
  global          This help text
  add             More detailed explanation on the add command
  install         More detailed explanation on the install command
  init            More detailed explanation on the init command
  repository      More detailed explanation on the repository command
```

Installation
------------

To install dep, build it from source and place the binary in your `$PATH`:

```sh
make
sudo make install
```

To install the binary in a location other than `/usr/local/bin`, pass the
`DESTDIR` definition to the `make install` command (default: `/usr/local`).

By default, no default repositories are enabled. You'll need to add your own
or use GitHub directly.

Usage
-----

### Initializing a project

To start using dep in your project, run the init command to create a `.dep` file:

```sh
dep init
```

This creates an empty `.dep` file in the current directory. You can also specify
a target directory:

```sh
dep init /path/to/project
```

### Adding a dependency

To add a package, you can call the following command:

```sh
dep add owner/library
dep add owner/library version
```

If a version is not specified, the latest version from the repository will be
used, or the default branch from GitHub.

Examples:

```sh
dep add finwo/palloc           # Latest from repo or GitHub main branch
dep add finwo/palloc edge     # Specific version/branch
dep add finwo/palloc v1.0.0   # Specific tag
```

You can also add a dependency with a direct URL:

```sh
dep add mylib https://example.com/mylib.tar.gz
```

### Installing dependencies

To install all dependencies listed in your `.dep` file:

```sh
dep install
```

Dependencies are installed to the `lib/` directory by default.

### Repository management

dep can use custom repositories to discover packages. Repositories are
configured in `~/.config/finwo/dep/repositories.d/`.

To add a repository:

```sh
dep repository add myorg https://example.com/path/to/manifest
```

To list configured repositories:

```sh
dep repository list
```

To remove a repository:

```sh
dep repository remove myorg
```

Creating Repositories
--------------------

Anyone can create a repository to host their own package manifests. A
repository is simply a static file server hosting a manifest file.

The manifest is a text file with one package per line. Lines starting with
`#` are comments. Each line has the following format:

```
name@version url
```

The version is optional. If omitted, the package is available without a
specific version.

Example manifest:

```
# My organization's packages
finwo/palloc@edge https://github.com/finwo/palloc/archive/refs/heads/edge.tar.gz
finwo/palloc@v1.0.0 https://github.com/finwo/palloc/archive/refs/tags/v1.0.0.tar.gz
finwo/palloc https://github.com/finwo/palloc/archive/refs/heads/main.tar.gz
myorg/mylib https://example.com/mylib-v1.0.0.tar.gz
myorg/mylib@2.0.0 https://example.com/mylib-v2.0.0.tar.gz
```

Host the manifest file on any static file server (GitHub Pages, S3, nginx,
etc.) and add the URL using `dep repository add`.

Building
--------

Building this dependency manager requires:
- clang (or your preferred C compiler)
- make

Simply run:

```sh
make
```

License
-------

This project falls under the [FGPL license](LICENSE.md)
