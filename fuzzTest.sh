#/usr/bin/sh

FAILURE_COUNT=0

if [ ! -d "./failures" ]; then
	mkdir ./failures
fi

while [ $FAILURE_COUNT -lt 100 ]; do
	breadtext --fuzz
	if [ -f "./failure.txt" ]; then
		mv ./failure.txt "./failures/failure${FAILURE_COUNT}.txt"
		((FAILURE_COUNT++))
	fi
	sleep 1
done

echo "Finished fuzz testing."
