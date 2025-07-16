# c-smf

## Overview
Platform-independent port of the Zephyr Real-Time Operating System (RTOS) State Machine Framework.

## Features
Easy integration with CMake.

Utilizes the powerful Zephyr State Machine Framework for efficient state management.

Apache 2.0 licensed for open-source use.

## Getting Started

1. Clone the repository
```
git clone https://github.com/kr-t/c-smf.git
```

2. Add the new directory to your CMake project and link smf library to your executable
```
add_subdirectory(c-smf)
target_link_libraries(... smf)
```

3. Include the State Machine Framework in your code
```
#include "smf/smf.h"
```

## Example 

Simple stand-alone example is available in /test folder. To build it, use BUILD_TESTS option.
```
mkdir build && cd build
cmake -DBUILD_TESTS=ON ..
make
./test/test
```

Output:
```
Entering State 1
Starting State Machine
Running State 1
Transitioning to State 2
Exiting State 1
Entering State 2
Running State 2
State Machine Test Complete
```

For more information, you can refer to the official Zephyr RTOS documentation for the State Machine Framework: [Zephyr SMF Documentation](https://docs.zephyrproject.org/latest/services/smf/index.html)

## License
This project is licensed under the Apache License 2.0 - see the LICENSE file for details.

## Acknowledments
This project is ported from Zephyr Real-Time Operating System.

Thanks to the open-source community for their contributions.
