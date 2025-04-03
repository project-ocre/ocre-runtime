# $ \frac{Config}{(dot) JSON} $ File Format

```json
{
    "name": "Name of Test Suite",
    "description": "Helpful description of the test suite",
    "setup": [{
        "name": "Name of script",
        "exec": "execution command"
    }],
    "tests": [{
        "name": "Name of test",
        "description": "Quick description",
        "exec": "execution command"
    }, {
        "name": "Name of test group",
        "description": "Quick description",
        "tests": [{
            "name": "Name of test",
            "description": "Quick description",
            "exec": "execution command"
        }]
    }],
    "cleanup": [{
        "name": "Name of script",
        "exec": "execution command"
    }]
}
```

Execution commands are run with `bash -c "execution command"`.
