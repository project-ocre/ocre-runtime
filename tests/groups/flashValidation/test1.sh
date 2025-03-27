read -p "Should this test pass? [y/n] " -n 1 -r
echo

if [[ $REPLY =~ ^[Yy]$ ]]; then
    echo "Test passed"
    exit 0
fi

echo "Test failed"
exit 1