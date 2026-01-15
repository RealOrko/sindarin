# Winget Package for Sindarin

This directory contains manifest templates for publishing Sindarin to the Windows Package Manager (winget).

## Prerequisites

- A GitHub release with the Windows ZIP artifact
- The SHA256 hash of the ZIP file

## Steps to Submit

### 1. Calculate SHA256 hash

After creating a release, calculate the hash:

```powershell
# Download the release
$version = "0.0.9"  # Replace with actual version
$url = "https://github.com/RealOrko/sindarin/releases/download/$version/sindarin-$version-windows-x64.zip"
Invoke-WebRequest -Uri $url -OutFile "sindarin.zip"

# Calculate SHA256
(Get-FileHash -Algorithm SHA256 "sindarin.zip").Hash
```

### 2. Update manifest files

Replace placeholders in all three manifest files:
- `${VERSION}` → actual version (e.g., `0.0.9`)
- `${SHA256}` → SHA256 hash from step 1

### 3. Fork and clone winget-pkgs

```bash
git clone https://github.com/YOUR_USERNAME/winget-pkgs.git
cd winget-pkgs
```

### 4. Create manifest directory

```bash
mkdir -p manifests/r/RealOrko/Sindarin/0.0.9
```

### 5. Copy manifests

Copy the three YAML files to the new directory:
- `RealOrko.Sindarin.yaml`
- `RealOrko.Sindarin.installer.yaml`
- `RealOrko.Sindarin.locale.en-US.yaml`

### 6. Validate manifests

```powershell
winget validate --manifest manifests/r/RealOrko/Sindarin/0.0.9
```

### 7. Test installation

```powershell
winget install --manifest manifests/r/RealOrko/Sindarin/0.0.9
```

### 8. Submit PR

Commit, push, and create a pull request to microsoft/winget-pkgs.

## Dependencies

The package declares these dependencies which winget will install automatically:
- `mstorsjo.llvm-mingw` - LLVM/Clang toolchain for Windows
- `Ninja-build.Ninja` - Ninja build system

## Automated Submission

Consider using [wingetcreate](https://github.com/microsoft/winget-create) to automate manifest creation:

```powershell
wingetcreate new https://github.com/RealOrko/sindarin/releases/download/0.0.9/sindarin-0.0.9-windows-x64.zip
```

Or update an existing package:

```powershell
wingetcreate update RealOrko.Sindarin --version 0.0.10 --urls https://github.com/RealOrko/sindarin/releases/download/0.0.10/sindarin-0.0.10-windows-x64.zip
```
