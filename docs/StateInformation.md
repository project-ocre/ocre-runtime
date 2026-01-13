# State information directory

## Location

The state information directory holds non-volatile data used by an Ocre context.

It can be customized by passing a different workspace directory to the `ocre_create_context` function is called.

When this function is called with `NULL` workspace, or when Ocre is installed as a system service in Linux or in zephyr, by default, it is defined as:

- Zephyr: `/lfs/ocre`
- Linux: `/var/lib/ocre`

## Structure

The structure of this directory is described below. Currently it holds two directories:

`images`
`containers`

### `images`

This directory holds the images available for local execution.

In the [Supervisor sample](samples/supervisor.md), this is managed by [Ocre CLI](OcreCli.md). The user can add or remove files in this directory but they must be careful to avoid race conditions.

### `containers`

Holds one subdirectory named after the container id, for each of the created containers with the `filesystem` runtime engine capability. In this case, the container subdirectory here will be mounted as the root filesystem inside the container.

This is used to provide a non-volatile storage to the container. It gets removed when the container is removed (but not on container restart).

This directory is managed by Ocre and the user is not expected to manually edit or modify files there, especially when the container is running.
