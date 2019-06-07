#/usr/bin/sh

FAILURE_COUNT=0

if [ ! -d "./failures" ]; then
	mkdir ./failures
fi

while [ $FAILURE_COUNT -lt 100 ]; do
	./build/breadtext --fuzz
	if [ -f "./failure.txt" ]; then
		mv ./failure.txt "./failures/failure${FAILURE_COUNT}.txt"
		((FAILURE_COUNT++))
	fi
done

echo "Finished fuzz testing."
