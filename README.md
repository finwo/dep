# DEP

general-purpose project dependency manager

---

## Installation

To install dep onto your machine, first verify the `/usr/local/bin` folder is in your `$PATH` variable.

After that, run the following command as root to install the dep scripts onto your machine:

```bash
curl https://raw.githubusercontent.com/cdeps/dep/master/install.sh | bash
```

---

## Usage

### Initialize a project with DEP

```bash
dep init
```

Go into the repository you want to initialize and run `dep init`. This will create the file `.dep` which contains all that is needed for dep to work with.

### Add a dependency

```bash
dep add <package-name>
```

### Install dependencies

```bash
dep install
```

You can also install a single package without adding it to the dependencies by running `dep install <package-name>`

### Remove a dependency

This is not implemented yet

### Update packages from repositories

```bash
dep repo update
```

### Show which repositories are used

```bash
dep repo list
```

### Add a repository

```bash
dep repo add <url>
```

### Clean repository cache

```bash
dep repo clean
```

## TODO

- Add remove-dep tool
- Make the source nicer to read
- Better documentation
- Pick a proper name
- Focus on C instead of general purpose?

## Contribute

TODO: No Code of Conduct
TODO: patreon page
