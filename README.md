# Windows Installer Foundation

This directory contains the foundation for building and distributing the Nirqoba compiler as a Windows application.

## Contents

- `build_scripts/` - Batch and PowerShell scripts for building the Nirqoba compiler on Windows
- `installer_scripts/` - Inno Setup or NSIS script templates for creating Windows installers
- `dist/` - Distribution outputs (installers, portable ZIPs)
- `licenses/` - License files and third-party attributions

## Getting Started

1. Ensure the Nirqoba compiler is built (see the main project's `README.md`)
2. Run the build scripts in `build_scripts/` to generate the compiler binaries
3. Use the installer scripts in `installer_scripts/` to create the Windows installer

## Phase 6.9 - Pre-Phase 7 Foundation

This phase establishes the Windows deployment infrastructure that will support:
- Phase 7: Full Windows application deployment and user experience
- Phase 8: Auto-update mechanisms
- Phase 9: Cross-platform binary distribution

### Completed in this phase:
- [x] Git repository initialization
- [x] Basic directory structure
- [x] README documentation
- [x] Build script templates
- [x] Installer script templates