## Overview

In the Ocre testing framework, the Ocre execution is comprised of test groups, test suites, and test cases.

Test groups represent a group of test suites which have the same environmental prerequisites, and require a 
common setup and teardown process, e.g. the Flash Validation test group requires that the DUT board has been
flashed with the Ocre runtime. Test groups will block each other on failure, meaning that if a test group
fails, all subsequent test groups will be skipped.

Test suites act as a way to logically split up test cases. For instance, in a test group for container workloads,
Test suite A might contain testcases for single container workloads, and test suite B might contain testcases for
multi-container workloads. Test suites are non blocking on one another on failure.

Test cases are the individual tests, and are literally a python or bash script (or any other language) which runs
some arbitrary code according to the test plan, and then exits / returns an exit code of 0 on success and 1 on
failure.

## Adding Test Groups, Test Suites, and Test Cases.

To add a test group, you must create a new directory under the /tests/ dir, named after the new test group. Then, 
add a config.json file in the directory according to the format given below. Finally, add a step to the
/.github/workflows/tests.yml file which runs the beginTests.sh against your new test group. 

To add a new testcase, the only requirement is that the testcase script file be placed under a testgroup dir, and
that it exits with 0 on success and 1 on failure.


```json
{
  "name": "Name of test group",
  "description": "Helpful description of the test group",
  "setup": [
    {
      "name": "Name of script",
      "exec": "execution command"
    }
  ],
  "test_suites": [
    {
      "name": "Name of test suite",
      "description": "Quick description",
      "test_cases": [
        {
          "name": "Name of test case",
          "description": "Quick description",
          "exec": "execution command"
        }
      ]
    }
  ],
  "cleanup": [
    {
      "name": "Name of script",
      "exec": "execution command"
    }
  ]
}
```

Execution commands are run with `bash -c "execution command"`.
