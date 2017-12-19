#/usr/bin/sh

TEST_COUNT=0
FAILURE_COUNT=0

if [ ! -d "./scriptTestResults" ]; then
	mkdir ./scriptTestResults
fi

echo " "

for FILE in ./scriptTests/*; do
	DESTINATION="./scriptTestResults/testResult${TEST_COUNT}.txt"
	echo "Running test:"
	echo $FILE
	echo $DESTINATION
	breadtext --script-test $FILE
	if [ $? -eq 0 ]; then
		echo "PASSED"
	else
		echo "FAILED"
		((FAILURE_COUNT++))
	fi
	echo " "
	mv ./scriptTestResult.txt $DESTINATION
	((TEST_COUNT++))
done

echo "================================="
echo "FAILED TESTS: ${FAILURE_COUNT} / ${TEST_COUNT}"
echo " "

