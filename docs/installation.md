# Installation

## Source

```sh
git clone https://github.com/VersaNexusIX/vpi
cd vpi
```

## Dependencies

### Debian / Ubuntu

```sh
apt install gcc libcurl4-openssl-dev zlib1g-dev make
```

### Fedora / RHEL

```sh
dnf install gcc libcurl-devel zlib-devel make
```

### Arch Linux

```sh
pacman -S gcc curl zlib make
```

### Termux (Android)

```sh
pkg install gcc libcurl zlib make
```

### macOS

```sh
xcode-select --install
brew install curl
```

zlib is bundled with Xcode Command Line Tools.

### Windows

Use MSYS2 MinGW64:

```sh
pacman -S mingw-w64-x86_64-gcc mingw-w64-x86_64-curl mingw-w64-x86_64-zlib make
```

## Build

```sh
make
make install
```

The binary is copied to `~/vpi` by default.

To change the destination:

```sh
make install DESTDIR=/usr/local/bin
```

To uninstall:

```sh
make uninstall
```

## PATH

To run `vpi` from any directory without the `~/` prefix:

```sh
echo 'export PATH="$HOME:$PATH"' >> ~/.bashrc
source ~/.bashrc
```

For Zsh:

```sh
echo 'export PATH="$HOME:$PATH"' >> ~/.zshrc
source ~/.zshrc
```

## Verify

```sh
vpi version
```

## CMake (Alternative)

```sh
mkdir build && cd build
cmake .. -DCMAKE_BUILD_TYPE=Release
make
make install
```
