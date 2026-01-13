# Devcontainers

Devcontainers are tools that allows you to create and manage isolated development environments in containers. It provides a consistent and reproducible development environment across different machines and operating systems.

It is the fastest and easiest way to develop Ocre in Linux, macOS, and Windows. As it already comes preinstalled with all the necessary tools for building Ocre and the samples, including wasi-sdk.

More information can be found at [the official Devcontainers documentation](https://containers.dev/).

## Usage

It is recommended to use VS Code devcontainers extension, however if your IDE does not support devcontainers, you can use the [devcontainer-cli](https://github.com/devcontainers/cli) tool.

### VS Code

Install the devcontainer extension for VS Code. Open the root of this repository.

Select "Reopen in Container" in the pop-up notification or select "Reopen in container" in the command pallete.

The select `linux` or `zephyr` from the command palette. The container will be built and started.

More information can be found at [VS code extension](https://code.visualstudio.com/docs/devcontainers/containers).

### devcontainer-cli

If your IDE does not support devcontainers, you can use the devcontainer-cli tool.

Install the devcontainer-cli:

```sh
npm install -g @devcontainers/cli
```

Open a terminal in the root of this repository.

Build the devcontainer. This needs to be done only once.

In the next steps, you can replace `.devcontainer/linux/devcontainer.json` with `.devcontainer/zephyr/devcontainer.json` if you want to use the Zephyr devcontainer.

```sh
devcontainer build --workspace-folder . --config .devcontainer/linux/devcontainer.json
```

Start the devcontainer. This can be done whenever the container needs to be started.

```sh
devcontainer up --workspace-folder . --config .devcontainer/linux/devcontainer.json
```

Execute a shell inside the devcontainer.

```sh
devcontainer exec --workspace-folder . --config .devcontainer/linux/devcontainer.json bash -i
```

For Linux, you get a shell like:

```
ubuntu@a5011ec9afd5:/workspaces/ocre$
```

For Zephyr, you get a shell like:

```
(zephyr-venv) ubuntu@a5011ec9afd5:/workspaces/ocre$
```

Note the Python venv is already activated.

To continue development, follow the instructions for Ocre build and development:

- [Get started with Linux](GetStartedLinux.md)
- [Get started with Zephyr](GetStartedZephyr.md)
