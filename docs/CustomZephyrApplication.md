## Adding Ocre Zephyr module to a custom Zephyr application

Ocre comes with a few samples to get you going fast:

- [`src/samples/mini/zephyr`](samples/mini.md)
- [`src/samples/demo/zephyr`](samples/demo.md)
- [`src/samples/supervisor/zephyr`](samples/supervisor.md)

However in most real-world scenarios, you will want to create your own application that controls Ocre.

This will guide you through the process of adding Ocre to an already existing Zephyr application.

Assuming you have a working zephyr build workspace, for example:

```sh
cd ~/zephyrproject
ls -1a
```

Output includes:

```
.west
zephyr
modules
```

You can add Ocre as a Zephyr module alongside the other directories (or anywhere else):

```sh
cd ~/zephyrproject
git clone --recursive https://github.com/project-ocre/ocre-runtime.git
```

And we can add include the Ocre Zephyr module in the build:

```sh
export EXTRA_ZEPHYR_MODULES=~/zephyrproject/ocre-runtime
```

Note that the ocre command line shell interface is not part of ocre-runtime, but part of the supervisor sample.

You probably want your application to control Ocre, being able to `#include <ocre/ocre.h>` and make calls to its API, so you need to tell Zephyr build system to make Ocre Runtime available to your application:

Edit `CMakeLists.txt` on your application and add:

```
target_link_libraries(app
    PUBLIC
    OcreCore
)
```

And you can continue building your application normally, which should be able to use Ocre.

See [Get Started with Zephyr](GetStartedZephyr.md) for more information.

Also, check the API documentation for more information on how to use Ocre in your application.
