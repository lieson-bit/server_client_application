#!/bin/bash

# Configuration
SERVER_PORT=20311  # Correct port for your server
SERVER_EXEC="./server"
TASKID_FILE="TASKID.txt"
TEST_FILE="sample.txt"
RESULTS_FILE="results.txt"
TEST_RESULTS_FILE="test_results.txt"  # File to store the test results

WORDS=("apple" "orange" "banana" "grape" "cherry")

# Step 1: Verify TASKID.txt
if [[ ! -f "$TASKID_FILE" ]]; then
    echo "Error: $TASKID_FILE not found."
    exit 1
fi

TASKID=$(head -n 1 "$TASKID_FILE")
if [[ "$TASKID" != "24" ]]; then
    echo "Error: TASKID.txt does not contain 24 as the first line."
    exit 1
fi

echo "TASKID.txt verified successfully."

# Step 2: Start the server
echo "Starting the server..."
$SERVER_EXEC $SERVER_PORT &
SERVER_PID=$!
sleep 1

# Check if the server started
if ps -p $SERVER_PID > /dev/null; then
    echo "Server started successfully."
else
    echo "Error: Server did not start."
    exit 1
fi

# Step 3: Test word counting
echo "Testing word counting..."
ALL_TESTS_PASSED=true

# Clear previous test results
> "$TEST_RESULTS_FILE"

for word in "${WORDS[@]}"; do
    # Send word and capture the response
    echo "$word $TEST_FILE" | nc -u -w1 172.18.182.131 $SERVER_PORT > temp_results.txt
    
    # Add a line to debug the output of the command
    # echo "Debug: Sent '$word $TEST_FILE', received output:" 
    # cat temp_results.txt  # Print the contents of temp_results.txt to debug
    
    # Parse and normalize the response
    RESPONSE=$(cat temp_results.txt | sed -E "s/'?([a-zA-Z]+)'?: ([0-9]+)/\1 \2/")

    # Append the normalized response to test_results.txt
    echo "$RESPONSE" >> "$TEST_RESULTS_FILE"
done

# Step 4: Ensure consistent newlines
# Append newline to test_results.txt if missing
if [[ -n $(tail -c 1 "$TEST_RESULTS_FILE") ]]; then
    echo "" >> "$TEST_RESULTS_FILE"
fi

# Append newline to results.txt if missing
if [[ -n $(tail -c 1 "$RESULTS_FILE") ]]; then
    echo "" >> "$RESULTS_FILE"
fi

# Step 5: Compare results using Python
echo "Comparing results using Python..."
PYTHON_COMPARISON_SCRIPT=$(cat <<EOF
import sys

def compare_files(file1, file2):
    try:
        with open(file1, 'r') as f1, open(file2, 'r') as f2:
            content1 = f1.readlines()
            content2 = f2.readlines()

        if content1 == content2:
            print("The files are identical.")
            sys.exit(0)
        else:
            print("The files are different.")
            print("\\nDifferences:")
            for i, (line1, line2) in enumerate(zip(content1, content2), start=1):
                if line1 != line2:
                    print(f"Line {i}:")
                    print(f"File1: {repr(line1)}")
                    print(f"File2: {repr(line2)}")

            if len(content1) > len(content2):
                print(f"File1 has extra lines starting from line {len(content2) + 1}:")
                print("".join(content1[len(content2):]))
            elif len(content2) > len(content1):
                print(f"File2 has extra lines starting from line {len(content1) + 1}:")
                print("".join(content2[len(content1):]))

            sys.exit(1)

    except FileNotFoundError as e:
        print(f"Error: {e}")
        sys.exit(1)

if __name__ == "__main__":
    file1 = "$TEST_RESULTS_FILE"
    file2 = "$RESULTS_FILE"
    compare_files(file1, file2)
EOF
)

python3 -c "$PYTHON_COMPARISON_SCRIPT"

if [[ $? -eq 0 ]]; then
    echo -e "\e[32mAll tests passed! OK\e[0m"  # Green 'OK' for success
else
    echo -e "\e[31mTest failed. The files are different.\e[0m"  # Red 'FAILED'
    ALL_TESTS_PASSED=false
fi

# Step 6: Shut down the server
kill $SERVER_PID
echo "Server stopped."

# Final result
if [[ "$ALL_TESTS_PASSED" == true ]]; then
    exit 0
else
    exit 1
fi
