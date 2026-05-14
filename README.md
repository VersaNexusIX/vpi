# VPI

Versa Package Installer. A cross-platform package manager for the Versa registry at versas.my.id.

Packages are distributed as `.vers` files, which are standard ZIP archives containing a `vers.json` manifest. VPI resolves package filenames through the registry index at `versas.my.id/vers.json` before downloading, so package names are always authoritative from the registry regardless of the filename on disk.

## Source

https://github.com/VersaNexusIX/vpi

## Requirements

- GCC or Clang
- libcurl (development headers)
- zlib (development headers)
- make

## Build

```sh
git clone https://github.com/VersaNexusIX/vpi
cd vpi
make
make install
```

Default install destination is `~/vpi`. Override with:

```sh
make install DESTDIR=/custom/path
```

### Platform Dependencies

| Platform | Command |
|---|---|
| Debian / Ubuntu | `apt install gcc libcurl4-openssl-dev zlib1g-dev make` |
| Fedora / RHEL | `dnf install gcc libcurl-devel zlib-devel make` |
| Arch | `pacman -S gcc curl zlib make` |
| Termux | `pkg install gcc libcurl zlib make` |
| macOS | `xcode-select --install && brew install curl` |

## Commands

| Command | Description |
|---|---|
| `vpi install <package>` | Install a package from versas.my.id |
| `vpi remove <package>` | Remove an installed package |
| `vpi update <package>` | Update a package to the latest version |
| `vpi list` | List all installed packages |
| `vpi view <package>` | Show details of an installed package |
| `vpi search <query>` | Search the registry |
| `vpi info <package>` | Show remote package metadata |
| `vpi init` | Create a vers.json interactively |
| `vpi cache` | Manage local download cache |
| `vpi version` | Show VPI version |
| `vpi help` | Show usage |

## Options

| Option | Description |
|---|---|
| `--verbose` | Detailed output |
| `--no-color` | Disable ANSI color |
| `--force` | Skip confirmation prompts |
| `--dry-run` | Simulate without making changes |

Flags may be placed anywhere in the command line:

```sh
vpi install neodax --force
vpi --force install neodax
```

## Documentation

- [Installation](docs/installation.md)
- [Package Format](docs/package-format.md)
- [CLI Reference](docs/cli-reference.md)
- [Creating Packages](docs/creating-packages.md)
- [Contributing](docs/contributing.md)

## License

NeoDAX License 1.0 (Origin). See [LICENSE](LICENSE).
Copyright (c) 2026 VersaNexusIX.
