
# Top-Level Functions and Constants

## Basic Value Functions

`copy(value)` - Returns a shallow copy of the value.

`num(text)` - Converts a string into a number. Returns null if the text cannot be converted.

`buff(value)` - Converts a value into a buffer.

`str(value)` - Converts a value into a string.

`list(value)` - Converts a value into a list.

`type(value)` - Returns the type of the value.

## Sequence Functions

`len(value)` - Returns the number of items in the sequence or dictionary.

`ins(sequence, index, item)` - Inserts a value into the sequence.

`insSub(sequence, index, subsequence)` - Inserts a subsequence into the sequence.

`rem(sequence, index)` - Removes a value from the sequence.

`remSub(sequence, startIndex, endIndex)` - Removes a subsequence from the sequence.

`push(sequence, item)` - Appends a value to the end of the sequence.

`pushLeft(sequence, item)` - Appends a value to the start of the sequence.

`pushSub(sequence, subsequence)` - Appends a subsequence to the end of the sequence.

`pushSubLeft(sequence, subsequence)` - Appends a subsequence to the start of the sequence.

`pop(sequence)` - Removes a value from the end of the sequence.

`popLeft(sequence)` - Removes a value from the start of the sequence.

`popSub(sequence)` - Removes a subsequence from the end of the sequence.

`popSubLeft(sequence)` - Removes a subsequence from the start of the sequence.

`find(sequence, item, startIndex: null, endIndex: null, direction: 1, isCaseSensitive: true)` - Searches for a value in the sequence. Returns -1 if the value was not found.

`findSub(sequence, subsequence, startIndex: null, endIndex: null, direction: 1, isCaseSensitive: true)` - Searches for a subsequence in the sequence. Returns -1 if the subsequence was not found.

`sort(sequence, order: 1)` - Sorts the items of the sequence in-place.

## Miscellaneous Functions

`keys(dictionary)` - Returns a list of keys in the dictionary.

`hasKey(dictionary, key)` - Returns whether the dictionary contains a key.

`hasItem(value, item)` - Returns whether the sequence or dictionary contains an item.

`bindFunc(function, value)` - Binds the function to a value.

`charToPoints(character)` - Converts the character to a list of integer Unicode points.

`pointsToChar(pointList)` - Converts the list of integer Unicode points to a character.

## Type Constants

`TYPE_NULL`  
`TYPE_NUMBER`  
`TYPE_BUFFER`  
`TYPE_STRING`  
`TYPE_LIST`  
`TYPE_FUNCTION`  
`TYPE_DICTIONARY`  
`TYPE_TYPE`  
`TYPE_CLASS`  
`TYPE_OBJECT`
