# 1. Find all directories in ./groups

# 2. For each group:
# 2.1. Read config.json
# 2.2. Write to global log saying "Begining <JSON.name> (<JSON.description>)"
# 2.3. For every entry in <JSON.setup>, execute the script there
# 2.4. For every test, execute the test. if there is a fail, break out of the suite or test
# 2.5. Run cleanp scripts

# 3. Write logs and results back to GitHub workflow
