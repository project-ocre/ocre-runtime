echo "Will this test pass?"
if [[ $(($RANDOM % 2)) == 0 ]]; then
    echo "Test passed!"
    exit 0
fi

echo "Test failed"
exit 1