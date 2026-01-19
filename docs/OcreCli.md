# Ocre CLI

The ocre command line interface (CLI) is a powerful tool for interacting with the Ocre runtime. It provides a simple and intuitive way to manage and execute images within the Ocre environment.

It is designed to be familiar with docker command line, however there are some different options and behavior. And all the underlying mechanisms are simpler and different.

Currently, it is only supported in Zephyr, and is used by the [supervisor](samples/supervisor.md) sample.

Below is a description of the available commands and options.

## General Commands

### `help`

Display help message and usage information.

Usage: `ocre help`

### `version`

Display version information including build details.

Usage: `ocre version`

Displays:

- Ocre version
- Commit ID
- Build information
- Build date

### Global Options

Usage: `ocre [GLOBAL OPTIONS] COMMAND [COMMAND ARGS...]`

The current global options are defined:

```
  -v        Verbose mode
```

## Container Management

### `container create`

Creates a container in the Ocre context.

Usage: `ocre container create [options] IMAGE [ARG...]`

Options:

```
  -d                       Creates a detached container
  -n CONTAINER_ID          Specifies a container ID
  -r RUNTIME               Specifies the runtime to use
  -v VOLUME:MOUNTPOINT     Adds a volume to be mounted into the container
  -v /ABSPATH:MOUNTPOINT   Adds a directory to be mounted into the container
  -k CAPABILITY            Adds a capability to the container
  -e VAR=VALUE             Sets an environment variable in the container
```

Options '-v', '-e', and '-k' can be supplied multiple times.

Note: Mount destinations must be absolute paths and cannot be '/'. Source paths must also be absolute.

### `container run`

Creates and starts a container in the Ocre context.

Usage: `ocre container run [options] IMAGE [ARG...]`

Options:

```
  -d                       Creates a detached container
  -n CONTAINER_ID          Specifies a container ID
  -r RUNTIME               Specifies the runtime to use
  -v VOLUME:MOUNTPOINT     Adds a volume to be mounted into the container
  -v /ABSPATH:MOUNTPOINT   Adds a directory to be mounted into the container
  -k CAPABILITY            Adds a capability to the container
  -e VAR=VALUE             Sets an environment variable in the container
```

Options '-v', '-e', and '-k' can be supplied multiple times.

### `container start`

Starts a container in the Ocre context.

Usage: `ocre container start CONTAINER`

The container must be in CREATED or STOPPED status to be started.

### `container kill`

Kills a container in the Ocre context.

Usage: `ocre container kill CONTAINER`

The container must be in RUNNING status to be killed.

### `container wait`

Waits for a container to exit.

Usage: `ocre container wait CONTAINER`

Returns the exit code of the container once it has finished executing.

### `container ps`

Lists containers in the Ocre context.

Usage: `ocre container ps [CONTAINER]`

When called without arguments, lists all containers. When called with a container ID, shows information for that specific container.

### `container rm`

Removes a non-running, non-paused container from the Ocre context.

Usage: `ocre container rm CONTAINER`

The container must be in STOPPED, CREATED, or ERROR status to be removed.

## Image Management

### `image ls`

Lists images in local storage.

Usage: `ocre image ls [IMAGE]`

When called without arguments, lists all images. When called with an image ID, shows information for that specific image.

### `image pull`

Downloads an image from a remote repository to the local storage.

Usage: `ocre image pull URL [NAME]`

Downloads an image from the specified URL. If no name is provided, the image name is extracted from the URL path. The command will fail if an image with the same name already exists locally.

Currently, only http and https URLs are supported. If NAME is not provided, it will be extracted from the URL path if possible.

### `image rm`

Removes an image from local storage.

Usage: `ocre image rm IMAGE`

Warning: This command does not check if the image is currently in use by containers.

## Shortcuts

The following shortcut commands are available for convenience:

### Container Shortcuts

- `ps` → `container ps` - List containers
- `create` → `container create` - Create a container
- `run` → `container run` - Create and start a container
- `start` → `container start` - Start a container
- `kill` → `container kill` - Kill a container
- `rm` → `container rm` - Remove a container

### Image Shortcuts

- `images` → `image ls` - List images
- `pull` → `image pull` - Pull an image

## ID Validation

Both container and image IDs must follow specific naming conventions:

- Valid characters are lowercase alphanumeric [a-z0-9] plus underscore (\_), hyphen (-), and dot (.)
- Cannot start with a dot (.)

## Examples

```sh
# Pull an image
ocre pull http://example.com/myimage.tar myimage

# List images
ocre images

# Create and run a container
ocre run -d -n mycontainer -e ENV_VAR=value myimage /bin/sh

# List containers
ocre ps

# Wait for container to finish
ocre wait mycontainer

# Remove container
ocre rm mycontainer
```
