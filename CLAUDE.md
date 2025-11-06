# CLAUDE.md

This file provides guidance to Claude Code (claude.ai/code) when working with code in this repository.

## Repository Overview

This repository contains system extensions for Talos Linux, enabling additional functionality beyond the default Talos capabilities. Extensions are published as container images containing a `manifest.yaml` and a `rootfs` directory with binaries and configuration files that get bind-mounted into the Talos system.

## Build System Architecture

### bldr and Pkgfile

The build system uses **bldr** (github.com/siderolabs/bldr), a declarative build tool similar to Dockerfile but optimized for building system packages. All extensions are orchestrated through:

- **Pkgfile**: Root build configuration (format: `v1alpha2`, uses `syntax = ghcr.io/siderolabs/bldr:v0.5.4`)
- **pkg.yaml**: Per-extension build instructions defining sources, dependencies, build steps, and finalization
- **vars.yaml**: Version numbers and checksums for dependencies (supports Renovate auto-updates)
- **manifest.yaml.tmpl**: Extension metadata template (name, version, author, description, compatibility)

### Dependency System

Extensions use a **stage-based dependency system**. When declaring dependencies:

```yaml
dependencies:
  - stage: base
  - stage: some-dependency
    from: /rootfs  # CRITICAL: copies built artifacts from dependency's /rootfs to current stage's root
```

**Key insight**: The `from: /rootfs` directive copies files from a dependency stage's `/rootfs` to the current stage's root filesystem (`/`). Without this, pkg-config and other build tools cannot find the dependency's libraries and headers.

### Building Extensions

Build with `make <extension-name>` which:
1. Downloads bldr binary to `_out/bldr`
2. Runs `docker buildx build` with bldr as the build frontend
3. Evaluates the extension's version using `$(ARTIFACTS)/bldr eval --target $@ '{{.VERSION}}'`
4. Tags and optionally pushes the container image

**Common Make targets**:
- `make <extension-name>` - Build extension as Docker image (default: `PUSH=false`)
- `make <extension-name> PUSH=true` - Build and push to registry
- `make local-<extension-name> DEST=_out/<name>` - Extract extension contents locally for inspection
- `make <extension-name> PLATFORM=linux/amd64` - Build for specific platform (default: `linux/amd64,linux/arm64`)
- `make update-checksums` - Auto-update checksums after changing versions in vars.yaml (uses `git diff -U0`)

**Build customization**:
- `REGISTRY` - Container registry (default: `ghcr.io`)
- `USERNAME` - Registry username (default: `clementnuss` per Makefile line 16)
- `PLATFORM` - Target platforms (default: `linux/amd64,linux/arm64`)
- `PUSH` - Push to registry (default: `false`)

### Extension Structure

Every extension must have:

```
<category>/<extension-name>/
├── manifest.yaml.tmpl    # Extension metadata (templated with vars.yaml)
├── vars.yaml             # Version numbers and checksums
├── pkg.yaml              # Build instructions
└── [service-files].yaml  # Optional: containerized service definitions
```

**Subdirectories for multi-stage builds**: Complex extensions can have subdirectories with their own `pkg.yaml` files (e.g., `storage/nfs-utils/rpcbind/pkg.yaml`, `storage/nfs-utils/nfs-utils-statd/pkg.yaml`). These become intermediate build stages.

### pkg.yaml Structure

```yaml
name: extension-name
variant: scratch
shell: /bin/bash
dependencies:
  - stage: base
  - stage: dependency-name
    from: /rootfs
steps:
  - sources:
      - url: https://example.com/source.tar.gz
        destination: source.tar.gz
        sha256: <checksum>
        sha512: <checksum>
    env:
      SOURCE_DATE_EPOCH: {{ .BUILD_ARG_SOURCE_DATE_EPOCH }}
    prepare:
      - |
        tar -xf source.tar.gz --strip-components=1
    build:
      - |
        ./configure --prefix=/usr/local
        make -j $(nproc)
    install:
      - |
        mkdir -p /rootfs
        make install DESTDIR=/rootfs
        rm -rf /rootfs/usr/local/share/man
    test:
      - |
        /extensions-validator validate --rootfs=/extensions-validator-rootfs --pkg-name="${PKG_NAME}"
    sbom:
      outputPath: /rootfs/usr/local/share/spdx/<name>.spdx.json
      version: {{ .SOME_VERSION }}
      licenses:
        - GPL-2.0
finalize:
  - from: /rootfs
    to: /rootfs
  - from: /pkg/manifest.yaml
    to: /
```

**Critical details**:
- Install to `/rootfs/<path>` - all files must be under `/rootfs` in the build stage
- The `finalize` section copies `/rootfs` from the build stage to the output image's `/rootfs`
- Main extension `pkg.yaml` must copy from dependencies: `cp -r /usr/local/sbin/* /rootfs/usr/local/sbin/`

### Permitted rootfs Paths

Extensions can only install files under these hierarchies:
- `/etc/cri/conf.d/` - Container runtime configuration
- `/usr/lib/firmware/` - Firmware binaries
- `/usr/lib/modules/` - Kernel modules
- `/usr/local/` - Binaries, libraries, etc.
- `/usr/local/etc/containers/` - Containerized service definitions
- `/var/lib/` - State directories
- `/run/` - Runtime files

## Creating a Custom Talos Installer Image

To use an extension, build a custom Talos installer image with the **imager** container:

```bash
# Build and push extension
make <extension-name> PUSH=true REGISTRY=ghcr.io USERNAME=yourusername

# Create custom installer with extension
docker run --rm -t \
  -v /var/run/docker.sock:/var/run/docker.sock \
  -v $PWD/_out:/out \
  ghcr.io/siderolabs/imager:v1.12.0 \
  installer \
  --arch amd64 \
  --system-extension-image ghcr.io/yourusername/<extension-name>:<version> \
  --output-kind docker

# Tag and push custom installer
docker tag <image-id> ghcr.io/yourusername/talos-installer:v1.12.0-<extension-name>
docker push ghcr.io/yourusername/talos-installer:v1.12.0-<extension-name>

# Upgrade Talos with custom installer
talosctl upgrade --image ghcr.io/yourusername/talos-installer:v1.12.0-<extension-name>
```

## Extension Tiers

- **:green_square: core** - Fully supported by Sidero Labs
- **:yellow_square: extra** - Best-effort support by Sidero Labs
- **:white_large_square: contrib** - Community supported

## Dependency Sharing

**Avoid duplicating packages**. Reuse existing stages from other extensions:

```yaml
dependencies:
  - stage: libevent
    from: /rootfs
    # This references storage/nfsrahead/libevent/pkg.yaml
```

Common shared dependencies in this repo:
- `libtirpc-zfs` (from `storage/zfs/zfs-tools/libtirpc`)
- `libevent` (from `storage/nfsrahead/libevent`)
- `sqlite` (from `storage/nfsrahead/sqlite`)

## Adding New Extensions

1. Create directory: `<category>/<extension-name>/`
2. Create `vars.yaml` with version numbers and checksums
3. Create `manifest.yaml.tmpl` with metadata
4. Create `pkg.yaml` with build instructions
5. Add extension name to `TARGETS` list in Makefile (alphabetically sorted)
6. Build and test: `make local-<extension-name> DEST=_out/test PLATFORM=linux/amd64`
7. Verify contents: inspect `_out/test/rootfs/` and `_out/test/manifest.yaml`

## Common Issues and Solutions

### Missing Dependencies
**Symptom**: `pkg-config` or `configure` can't find a library
**Fix**: Ensure dependency has `from: /rootfs` directive and the dependency's pkg.yaml installs to `/rootfs`

### Missing Binaries in Output
**Symptom**: `make local-<extension>` output is missing binaries
**Fix**: Main extension pkg.yaml must copy from dependencies: `cp -r /usr/local/sbin/* /rootfs/usr/local/sbin/`

### Checksum Mismatches
**Fix**: Download the actual source tarball, compute checksums:
```bash
curl -L <url> -o file.tar.gz
sha256sum file.tar.gz
sha512sum file.tar.gz
```

### Auto-update Checksums
After changing version numbers in `vars.yaml`:
```bash
make update-checksums
```
This uses `bldr update` to automatically download sources and update checksums.

## Containerized Services

Extensions can provide containerized services (similar to systemd units but for Talos). Service YAML files define dependencies, entrypoints, arguments, and mounts:

```yaml
name: service-name
depends:
  - service: cri
  - network:
      - addresses
      - connectivity
container:
  entrypoint: /usr/local/sbin/daemon
  args:
    - --foreground
  mounts:
    - source: /usr/local/sbin
      destination: /usr/local/sbin
      type: bind
      options:
        - bind
        - ro
restart: always
```

Install to: `/rootfs/usr/local/etc/containers/<service-name>.yaml`

## Key Files

- `Makefile` - Main build orchestration (line 97: TARGETS list)
- `Pkgfile` - Root bldr configuration with global variables
- `hack/catalog.template` - Template for generating README extension catalog
- `internal/extensions/descriptions.yaml` - Auto-generated metadata from built extensions
