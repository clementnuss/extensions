# Joke Daemon Extension

An example extension that demonstrates how to build and deploy a simple daemon in Talos Linux.

## Overview

This extension showcases the complete process of creating a Talos Linux extension from scratch, including:

- Compiling a simple C program
- Packaging it as an extension with a manifest
- Creating a containerized service that runs automatically
- Building and testing the extension

The daemon prints random programming jokes to stdout every 60 seconds.

## Structure

```
examples/joke-daemon/
├── joke-daemon.c          # Source code for the daemon
├── joke-daemon.yaml       # Containerized service definition
├── manifest.yaml.tmpl     # Extension metadata template
├── pkg.yaml               # Build instructions
├── vars.yaml              # Version information
└── README.md             # This file
```

## Building

Build the extension locally to inspect its contents:

```bash
make local-joke-daemon DEST=_out/joke-daemon PLATFORM=linux/amd64
```

Build and push the extension as a container image:

```bash
make joke-daemon PUSH=true REGISTRY=ghcr.io USERNAME=yourusername
```

## Using the Extension

To use this extension in your Talos cluster, you need to create a custom installer image that includes it:

1. First, build and push the extension (see above)

2. Create a custom Talos installer with the extension:

```bash
docker run --rm -t \
  -v /var/run/docker.sock:/var/run/docker.sock \
  -v $PWD/_out:/out \
  ghcr.io/siderolabs/imager:v1.12.0 \
  installer \
  --arch amd64 \
  --system-extension-image ghcr.io/yourusername/joke-daemon:1.0.0 \
  --output-kind docker
```

3. Tag and push the custom installer:

```bash
docker tag <image-id> ghcr.io/yourusername/talos-installer:v1.12.0-joke-daemon
docker push ghcr.io/yourusername/talos-installer:v1.12.0-joke-daemon
```

4. Upgrade your Talos nodes:

```bash
talosctl upgrade --image ghcr.io/yourusername/talos-installer:v1.12.0-joke-daemon
```

## Viewing Logs

Once the extension is installed and running, you can view the jokes in the service logs:

```bash
talosctl logs ext-joke-daemon
```

You should see output like:

```
JOKE: Why do programmers prefer dark mode? Because light attracts bugs!
JOKE: Why did the developer go broke? Because he used up all his cache!
```

## Educational Value

This example is ideal for teaching Talos extension development because it demonstrates:

1. **Simple Build Process**: Compiles a single C file with minimal dependencies
2. **Extension Structure**: Shows all required files (manifest, pkg.yaml, vars.yaml)
3. **Containerized Services**: Demonstrates how to create a service that runs automatically
4. **Proper Installation**: Places files in permitted rootfs paths (`/usr/local/lib/containers/<service-name>`, `/usr/local/etc/containers`)
5. **Clean Code**: Includes comments and proper error handling

## Key Concepts

### Build System (pkg.yaml)

The `pkg.yaml` file defines:
- Dependencies (uses the `base` stage)
- Build steps (compiles the C program)
- Install steps (copies files to `/rootfs`)
- Test steps (validates the extension)
- Finalize steps (packages the extension)

### Service Definition (joke-daemon.yaml)

The service file defines:
- Service dependencies (waits for CRI and network)
- Container entrypoint and arguments
- Mount points
- Restart policy

### Manifest (manifest.yaml.tmpl)

The manifest includes:
- Extension metadata (name, version, author)
- Description
- Compatibility requirements

## Customization

To adapt this example for your own extension:

1. Replace `joke-daemon.c` with your own program
2. Update `vars.yaml` with your version numbers
3. Modify `pkg.yaml` if you need different build steps or dependencies
4. Update `manifest.yaml.tmpl` with your extension details
5. Adjust `joke-daemon.yaml` service definition as needed
6. Add your extension to the `TARGETS` list in the root `Makefile`

## Notes

- The daemon is compiled statically (`-static` flag) to avoid runtime library dependencies
- The service runs in a container but mounts `/usr/local/bin` read-only
- The daemon handles SIGTERM and SIGINT gracefully for clean shutdown
