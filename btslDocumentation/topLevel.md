
# Top-Level Functions and Constants

## Basic Value Functions

`copy(value)` - Returns a shallow copy of the value.

* `value` may have any type except object or class.
* Return value has the same type as `value`.

`num(text)` - Converts a string into a number.

* `text` must be a string containing a decimal number.
* Return value is a number.
* Throws `ValueError` if `text` cannot be converted.

`buff(value)` - Converts a value into a buffer.

* `value` may be a number, buffer, string, or list of numbers.
* Return value is a buffer.

`str(value)` - Converts a value into a string.

* `value` may have any type.
* Return value is a string.

`list(value)` - Converts a value into a list.

* `value` may be a buffer, string, or list.
* If `sequence` is a buffer, return value will be a list of numbers.
* If `sequence` is a string, return value will be a list of characters.
* If `sequence` is a list, return value will be a list of values with mixed types.

`type(value)` - Returns the type of the value.

* `value` may have any type.
* Return value is a type.

## Sequence Functions

`len(value)` - Returns the number of items in the sequence or dictionary.

* `value` may be a buffer, string, list, or dictionary.
* Return value is a number.

`ins(sequence, index, item)` - Inserts a value into the sequence.

* `sequence` may be a buffer, string, or list.
* `index` must be a number.
* If `sequence` is a buffer, `item` must be a number.
* If `sequence` is a string, `item` must be a character.
* If `sequence` is a list, `item` may have any type.
* Does not return a value.
* Throws `ValueError` if `index` is out of range.

`insSub(sequence, index, subsequence)` - Inserts a subsequence into the sequence.

* `sequence` may be a buffer, string, or list.
* `index` must be a number.
* `subsequence` must have the same type as `sequence`.
* Does not return a value.
* Throws `ValueError` if `index` is out of range.

`rem(sequence, index)` - Removes a value from the sequence.

* `sequence` may be a buffer, string, or list.
* `index` must be a number.
* Does not return a value.
* Throws `ValueError` if `index` is out of range.

`remSub(sequence, startIndex, endIndex)` - Removes a subsequence from the sequence.

* `sequence` may be a buffer, string, or list.
* `startIndex` and `endIndex` must be numbers.
* Does not return a value.
* Throws `ValueError` if `startIndex` or `endIndex` are out of range.

`push(sequence, item)` - Appends a value to the end of the sequence.

`pushLeft(sequence, item)` - Appends a value to the start of the sequence.

* `sequence` may be a buffer, string, or list.
* If `sequence` is a buffer, `item` must be a number.
* If `sequence` is a string, `item` must be a character.
* If `sequence` is a list, `item` may have any type.
* Does not return a value.

`pushSub(sequence, subsequence)` - Appends a subsequence to the end of the sequence.

`pushSubLeft(sequence, subsequence)` - Appends a subsequence to the start of the sequence.

* `sequence` may be a buffer, string, or list.
* `subsequence` must have the same type as `sequence`.
* Does not return a value.

`pop(sequence)` - Removes a value from the end of the sequence.

`popLeft(sequence)` - Removes a value from the start of the sequence.

* `sequence` may be a buffer, string, or list.
* If `sequence` is a buffer, return value will be a number.
* If `sequence` is a string, return value will be a character.
* If `sequence` is a list, return value may have any type.
* Throws `ValueError` if `sequence` is empty.

`popSub(sequence, length)` - Removes a subsequence from the end of the sequence.

`popSubLeft(sequence, length)` - Removes a subsequence from the start of the sequence.

* `sequence` may be a buffer, string, or list.
* `length` must be a non-negative number.
* Return value has the same type as `sequence`.
* Throws `ValueError` if `sequence` is too small.

`find(sequence, item, startIndex: null, endIndex: null, direction: 1, isCaseSensitive: true)` - Searches for a value in the sequence. Returns -1 if the value was not found.

* `sequence` may be a buffer, string, or list.
* If `sequence` is a buffer, `item` must be a number.
* If `sequence` is a string, `item` must be a character.
* If `sequence` is a list, `item` may have any type.
* `startIndex` and `endIndex` may be numbers or null.
* `direction` must be a non-zero number.
* `isCaseSensitive` must be a boolean.
* Return value is a number.

`findSub(sequence, subsequence, startIndex: null, endIndex: null, direction: 1, isCaseSensitive: true)` - Searches for a subsequence in the sequence. Returns -1 if the subsequence was not found.

* `sequence` may be a buffer, string, or list.
* `subsequence` must have the same type as `sequence`.
* `startIndex` and `endIndex` may be numbers or null.
* `direction` must be a non-zero number.
* `isCaseSensitive` must be a boolean.
* Return value is a number.

`sort(sequence, order: 1, comparator: null)` - Sorts the items of the sequence in-place.

* `sequence` may be a buffer, string, or list.
* `order` must be a non-zero number.
* `comparator` may be null or a function with the signature `myFunc(value1, value2)`.
    * If `sequence` is a buffer, `value1` and `value2` will be numbers.
    * If `sequence` is a string, `value1` and `value2` will be characters.
    * If `sequence` is a list, `value1` and `value2` may have any type.
    * Return value must be a number.
* Does not return a value.

## Miscellaneous Functions

`keys(dictionary)` - Returns a list of keys in the dictionary.

* `dictionary` must be a dictionary.
* Return value is a list.

`hasKey(dictionary, key)` - Returns whether the dictionary contains a key.

* `dictionary` must be a dictionary.
* `key` may have any type.
* Return value is a boolean.

`hasItem(value, item)` - Returns whether the sequence or dictionary contains an item.

* `value` may be a buffer, string, list, or dictionary.
* If `value` is a buffer, `item` must be a number.
* If `value` is a string, `item` must be a character.
* If `value` is a list or dictionary, `item` may have any type.
* Return value is a boolean.

`bindFunc(function, value)` - Binds the function to a value.

* `function` must be a function.
* `value` may have any type.
* Return value is a function.

`charToPoints(character)` - Converts the character to a list of integer Unicode points.

* `character` must be a character.
* Return value is a list of numbers.

`pointsToChar(pointList)` - Converts the list of integer Unicode points to a character.

* `pointList` must be a list of numbers.
* Return value is a character.
* Throws `ValueError` if `pointList` does not describe a character.

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
