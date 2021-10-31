#/usr/bin/sh

TEST_COUNT=0
FAILURE_COUNT=0

if [ ! -d "./systematicTestResults" ]; then
	mkdir ./systematicTestResults
fi

echo " "

for FILE in ./systematicTests/test*.txt; do
	DESTINATION="./systematicTestResults/testResult${TEST_COUNT}.txt"
	echo "Running test:"
	echo $FILE
	echo $DESTINATION
	./build/breadtext --test $FILE
	if [ $? -eq 0 ]; then
		echo "PASSED"
	else
		echo "FAILED"
		((FAILURE_COUNT++))
	fi
	echo " "
	mv ./systematicTestResult.txt $DESTINATION
	((TEST_COUNT++))
done

echo "================================="
echo "FAILED TESTS: ${FAILURE_COUNT} / ${TEST_COUNT}"
echo " "

