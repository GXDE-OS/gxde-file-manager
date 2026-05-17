## gxde-file-manager

> **WARNING**: Installing this ported version will UNINSTALL your original version of `gxde-file-manager`, and this is a "work in progress" version, you shall consider if you really want to install the package or just conpile & run without installing!!

Deepin File Manager is a file management tool independently developed by Deepin Technology, featured with searching, copying, trash, compression/decompression, file property and other file management functions.

Now DFM V4.x is forked and maintained by GXDE to provide filemanager and desktop for GXDE

> **NOTE**: This branch is an attempt to migrate to Qt6.

## Qt6 Migration Status
### Overall Status
There are 2 functions that are currently UNAVAILABLE:
* **dde-video-preview-plugin**: Relys on `libgxmr` which is Qt5 only now.
* **DVideoWidget**: The header files exist in `dtk2widget-qt6`, but the symbols are NOT YET implemented.

### Package rename (`-neo` suffix)
To avoid conflicts with the upstream `gxde-file-manager` packages when installed side-by-side, every Debian binary/source package produced from this branch is suffixed with `-neo`. Whichever build system is used (QMake or CMake), the produced binaries land in the renamed deb packages — binary paths inside the packages (e.g. `/usr/bin/gxde-file-manager`, `libgxde-file-manager.so`) are unchanged.

| Original name | Renamed (`-neo`) |
| --- | --- |
| `gxde-file-manager` (Source) | `gxde-file-manager-neo` |
| `gxde-file-manager` | `gxde-file-manager-neo` |
| `gxde-desktop-panel` | `gxde-desktop-panel-neo` |
| `gxde-disk-mount-plugin` | `gxde-disk-mount-plugin-neo` |
| `libgxde-file-manager` | `libgxde-file-manager-neo` |
| `libgxde-file-manager-dev` | `libgxde-file-manager-neo-dev` |

Each `-neo` package declares `Provides`/`Replaces`/`Conflicts` against its original counterpart, so installing a `-neo` package transparently replaces the upstream one. Reverse-dependencies that reference `dde-file-manager` or the original `gxde-*` names continue to resolve.

Files changed for the rename:
* `debian/control` — `Source:`, all `Package:` stanzas, plus inter-package `Depends`/`Provides`/`Replaces`/`Conflicts`.
* `debian/changelog` — top entry's source name changed to `gxde-file-manager-neo`.
* `debian/*.install`, `debian/*.postinst` — renamed to match the new package names (incl. architecture-specific variants `*.install.arm64` / `*.install.mips*` / `*.install.sw_64`).

Files intentionally NOT changed:
* Installed binary/library paths inside the deb packages (would break `.desktop`, DBus services, polkit policies, library SONAME).
* `CMakeLists.txt` / `*.pro` — they build binaries, not Debian packages; CMake `project()` name is just a label.
* `debian/copyright` `Upstream-Name` — still refers to the upstream project.

### How to compile
```bash
mkdir -p ./build/all
cd ./build/all
qmake6 ../../filemanager.pro
make
```


### How to run without installing to system path
```bash
LD_LIBRARY_PATH=/home/char/Desktop/Repository/GXDE/FileMan/gxde-file-manager/build/all/gxde-file-manager-lib:$LD_LIBRARY_PATH /home/char/Desktop/Repository/GXDE/FileMan/gxde-file-manager/build/all/gxde-file-manager/gxde-file-manager
```

> **WARN**: Above command is an example, you must replace the paths with the real path. Also, prepare to get [dtk2widget-qt6](https://gitee.com/GXDE-OS/dtk2widget-qt6) and [gxde-qt6-integration](https://gitee.com/GXDE-OS/gxde-qt6-integration) installed to your system, as they provide DTK2 port to Qt6 world.

### Dependencies

### Build dependencies

_The **master** branch is current development branch, build dependencies may changes without update README.md, refer to `./debian/control` for a working build depends list_
 
* pkg-config
* dh-systemd
* libxcb1-dev
* libxcb-ewmh-dev
* libxcb-util0-dev
* libx11-dev
* libgsettings-qt-dev
* libsecret-1-dev
* libpoppler-cpp-dev
* libpolkit-agent-1-dev
* libpolkit-qt5-1-dev
* libjemalloc-dev
* libmagic-dev
* libtag1-dev
* libdmr-dev
* x11proto-core-dev
* libdframeworkdbus-dev
* gxde-dock-dev(>=4.0.5)
* deepin-gettext-tools
* libdtkcore-dev
* ffmpeg module(s):
  - libffmpegthumbnailer-dev
* Qt5(>= 5.6) with modules:
  - qtbase5-dev
  - qtbase5-private-dev
  - libqt5x11extras5-dev
  - qt5-qmake
  - libqt5svg5-dev
  - qttools5-dev-tools
  - qtmultimedia5-dev
  - qtdeclarative5-dev
  - libkf5codecs-dev
* Deepin-tool-kit(>=2.0) with modules:
  - libdtkwidget-dev
* deepin-anything with modules:
  - deepin-anything-dev
  - deepin-anything-server-dev

## Installation

### Build from source code

1. Make sure you have installed all dependencies.

_Package name may be different between distros, if gxde-file-manager is available from your distro, check the packaging script delivered from your distro is a better idea._

Assume you are using [Deepin](https://distrowatch.com/table.php?distribution=deepin) or other debian-based distro which got gxde-file-manager delivered:

``` shell
$ apt build-dep gxde-file-manager
```

2. Build:
```
$ cd gxde-file-manager
$ mkdir Build
$ cd Build
$ qmake ..
$ make
```

3. Install:
```
$ sudo make install
```

The executable binary file could be found at `/usr/bin/gxde-file-manager`

## Usage

Execute `gxde-file-manager`

## Documentations

 - [Development Documentation](https://linuxdeepin.github.io/gxde-file-manager/)
 - [User Documentation](https://wiki.deepin.org/wiki/Deepin_File_Manager) | [用户文档](https://wiki.deepin.org/index.php?title=%E6%B7%B1%E5%BA%A6%E6%96%87%E4%BB%B6%E7%AE%A1%E7%90%86%E5%99%A8)


## License

gxde-file-manager is licensed under [GPLv3](LICENSE)
