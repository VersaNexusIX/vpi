# CLI Reference

## Synopsis

```
vpi <command> [options] [arguments]
```

## Global Options

Flags may be placed anywhere in the command line, before or after arguments.

| Option | Description |
|---|---|
| `--verbose` | Print additional output including file lists and HTTP details |
| `--no-color` | Disable ANSI color codes |
| `--force` | Skip confirmation prompts and overwrite existing state |
| `--dry-run` | Simulate the operation without writing any files |

## Commands

### vpi install

Downloads and installs a package from the Versa registry.

```
vpi install <package>
```

Resolution order:

1. Fetch `versas.my.id/vers.json` and locate the entry by `nama`.
2. Read `_filename` from the entry to determine the download URL.
3. Download `versas.my.id/package/<_filename>`.
4. Extract, validate `vers.json`, install under the name from `nama`.

If the registry index is unreachable, falls back to `versas.my.id/package/<name>.vers`.

If the package is already installed, exits with a warning unless `--force` is provided.

### vpi remove

Removes an installed package and its metadata.

```
vpi remove <package>
```

Prompts for confirmation. Use `--force` to skip.

### vpi update

Reinstalls a package to the latest version from the registry.

```
vpi update <package>
```

The package must already be installed.

### vpi list

Displays all installed packages.

```
vpi list
```

Shows name, version, developer, and size. Use `--verbose` to also show license and install date.

### vpi view

Shows full metadata for an installed package.

```
vpi view <package>
```

Use `--verbose` to additionally list all installed files.

### vpi search

Queries the registry for packages matching a keyword.

```
vpi search <query>
```

Installed packages are marked in results.

### vpi info

Fetches and displays metadata for a remote package without installing it.

```
vpi info <package>
```

If the package is already installed, the current local version is shown alongside the remote version, and an update notice is printed if versions differ.

### vpi init

Interactively generates a `vers.json` in the current directory.

```
vpi init
```

Prompts for all required and optional fields. Validates semver format. Shows a preview before writing. Prompts before overwriting an existing file.

Use `--force` to overwrite without prompting. Use `--dry-run` to preview without writing.

### vpi cache

Manages the local download cache.

```
vpi cache
vpi cache clear
vpi cache size
```

| Subcommand | Description |
|---|---|
| (none) | Show cache path and total size |
| `clear` | Delete all cached files |
| `size` | Print cache size only |

### vpi version

Prints the installed VPI version.

```
vpi version
```

### vpi help

Prints the usage summary.

```
vpi help
```

## Exit Codes

| Code | Meaning |
|---|---|
| 0 | Success |
| 1 | Error (missing argument, package not found, network failure, validation failure) |
