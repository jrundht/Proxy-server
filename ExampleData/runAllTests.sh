#!/bin/bash

TEST_DIRS=(
  "./test-1-xml-to-xml"
  "./test-2-xml-to-2xml"
  "./test-3-2xml-to-xml"
  "./test-4-bin-to-bin"
  "./test-5-bin-to-2bin"
  "./test-6-2bin-to-bin"
  "./test-7-xml-to-xml+bin"
  "./test-8-mix-to-mix"
  "./test-9-sleepzero"
  "./test-10-xml-split"
  "./test-11-bin-split"
)

TEST_RESULTS=""

PASSED_COUNT=0
FAILED_COUNT=0


pids=() # Declare an array to store process IDs
statuses=() # Declare an array to store exit statuses


echo "" >test_results.md # Clear the test results file
echo "Test Results" >> test_results.md
echo "============" >> test_results.md
echo "" >> test_results.md
echo "Timestamp: $(date)" >> test_results.md
echo "" >> test_results.md
echo ""
echo "Starting all the tests..."
echo ""
for test_dir in "${TEST_DIRS[@]}"; do
  (


    cd "$test_dir" && chmod +x *.sh && sleep $(( RANDOM % 9 )) && ./RunTest.sh >/dev/null 2>&1

    file_pairs=(
      "X.xml X.xml-expected"
      "X.bin X.bin-expected"
      "Y.xml Y.xml-expected"
      "Y.bin Y.bin-expected"
      "Z.xml Z.xml-expected"
      "Z.bin Z.bin-expected"
    )

    for file_pair in "${file_pairs[@]}"; do
      files=($file_pair)
      if [[ -e "${files[0]}" && -e "${files[1]}" ]]; then
        diff_output=$(diff "${files[0]}" "${files[1]}")
        diff_exit_status=$?

        current_time=$(date)
      if [ "$diff_exit_status" -eq 0 ]; then
        echo -e "\n---\n" >> "../test_results.md"
        echo -e "### Test Directory: \`$test_dir\`" >> "../test_results.md"
        echo -e "**Checkfiles**: \`$test_dir/${files[0]}\` and \`$test_dir/${files[1]}\`" >> "../test_results.md"
        echo -e "**Status**: <span style=\"color:green;\">PASSED</span> - _Test ID_: $test_dir \n_Timestamp_: $current_time" >> "../test_results.md"
      else
        echo -e "\n---\n" >> "../test_results.md"
        echo -e "### Test Directory: \`$test_dir\`" >> "../test_results.md"
        echo -e "**Checkfiles**: \`$test_dir/${files[0]}\` and \`$test_dir/${files[1]}\`" >> "../test_results.md"
        echo -e "**Status**: <span style=\"color:red;\">FAILED</span> - _Test ID_: $test_dir \n_Timestamp_: $current_time" >> "../test_results.md"
        echo -e "\n**Differences**:\n" >> "../test_results.md"
        echo -e '```diff' >> "../test_results.md"
        echo -e "$diff_output" >> "../test_results.md"
        echo -e '```' >> "../test_results.md"
      fi

      fi
    done

    exit $diff_exit_status
  ) &
  pids+=("$!")
done

# Define the bar length and the characters to use
num_tests=${#pids[@]}
bar_length=20
chars=(' ' '.' '..' '...' '....' '.....' '......' '.......' '........' '.........')

# Run the progression bar
counter=0
while true; do
  active_tests=0
  for pid in "${pids[@]}"; do
    if kill -0 "$pid" 2>/dev/null; then
      ((active_tests++))
    fi
  done

  # Progression bar
  if [ $active_tests -eq 0 ]; then
    printf "\r[====================] 100%%\n"
    break
  else
    progress=$((100 - active_tests * 100 / ${#TEST_DIRS[@]}))
    char_index=$((progress * (${#chars[@]} - 1) / 100))

    # Print the progression bar
    bar_chars="${chars[char_index]}"
    printf "\r[%-${bar_length}s] %d%%   Running tests (%d/%d)" "$bar_chars" "$progress" "$((num_tests - active_tests))" "${#TEST_DIRS[@]}"
    sleep 0.1
    ((counter++))
  fi
done

printf "\r\n"

# Wait for all processes to complete and store exit statuses
for pid in "${pids[@]}"; do
  wait "$pid"
  exit_status=$?
  statuses+=("$exit_status")
done

# Print stored output messages
for output_message in "${output_messages[@]}"; do
  echo -e "$output_message"
done

# Count passed and failed tests
for status in "${statuses[@]}"; do
  if [ "$status" -eq 0 ]; then
    ((PASSED_COUNT++))
  else
    ((FAILED_COUNT++))
  fi
done


echo -e "Test Results: (See test_results.md for more details)"
echo "----------------------------------------"
echo -e "\033[32mPassed tests: $PASSED_COUNT\033[0m"
echo -e "\033[31mFailed tests: $FAILED_COUNT\033[0m"
