# Windows Installer Foundation

This directory contains a Windows packaging foundation for the Nirqoba compiler.

## Contents

- `build_scripts/build_msvc.bat` builds the compiler and stages a package.
- `build_scripts/stage_package.ps1` collects the compiler, runtime library, examples, and docs.
- `build_scripts/build_installer.bat` builds an installer from the staged package.
- `installer_scripts/nirqoba_installer.iss` defines the Inno Setup installer.


## Build

1. Install the pinned Windows build dependencies and run `scripts/bootstrap.ps1` in the repository root.
2. Install Inno Setup 6 and make `ISCC.exe` available on `PATH` to build an installer.
3. Run `build_scripts\build_msvc.bat Release` from this directory to build and stage the package.
4. Run `build_scripts\build_installer.bat` to create `dist\nirqoba-setup.exe`.

To stage an existing build without rebuilding, run
`powershell -NoProfile -ExecutionPolicy Bypass -File build_scripts\stage_package.ps1 -Configuration RelWithDebInfo`.

## Current limits

This is a developer packaging foundation, not a relocatable end-user compiler distribution.
`toolchain/toolchain.json` contains absolute paths to the build host's LLVM and MSVC installations.
The package includes the runtime static library for linking, so compilation on another machine still
requires a compatible toolchain installation and a generated configuration. A redistributable,
relocatable toolchain bundle remains future work.

The installer script and staging flow are implemented, but an actual installer build and install/
uninstall check require Inno Setup 6 (`ISCC.exe`) and have not yet been validated here.
