
# Built-In Values

## Top-Level Functions

`copy(value)` - Returns a shallow copy of the value.

`num(text)` - Converts a string into a number. Returns null if the text cannot be converted.

`buff(value)` - Converts a value into a buffer.

`str(value)` - Converts a value into a string.

`list(value)` - Converts a value into a list.

`type(value)` - Returns the type of the value.

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

`keys(dictionary)` - Returns a list of keys in the dictionary.

`hasKey(dictionary, key)` - Returns whether the dictionary contains a key.

`hasItem(value, item)` - Returns whether the sequence or dictionary contains an item.

`sort(sequence, order: 1)` - Sorts the items of the sequence in-place.

`bindFunc(function, value)` - Binds the function to a value.

`charToPoints(character)` - Converts the character to a list of integer Unicode points.

`pointsToChar(pointList)` - Converts the list of integer Unicode points to a character.

## Top-Level Constants

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

## Object Instance Methods

`myObject.getClass()` - Returns the class of the object.

`myObject.isInstanceOf(class)` - Returns whether the object is an instance of the given class.

`myObject.getSuper(class: null)` - Returns a version of the object with the given binding and overriding class. If the class is null, use the non-Object superclass. If the class is null, and there is not exactly one non-Object superclass, throw an error.

`myObject.compare(item)` - Returns -1, 0, or 1 depending on comparison of values. This method is used for comparison operators, dictionary keys, and the `sort` function.

`myObject.toString()` - Returns a string representation of the object and its values. This method is used by the `str` function.

## Object Class Methods

`Object.new()` - Creates an instance of the class.

`Object.getName()` - Gets the name of the class.

`Object.isSubclassOf(class)` - Returns whether the class is a subclass of the given class.

`Object.getSuper(class: null)` - Returns a version of the class with the given binding and overriding class. If the class is null, use the non-Object superclass. If the class is null, and there is not exactly one non-Object superclass, throw an error.

`Object.toString()` - Returns a string representation of the class and its values. This method is used by the `str` function.

## Iterable Class

`myIterable.getIterator()` - Returns an instance of `Iterator`. This method is used in `for` loops.

## Iterator Class

`myIterator.hasNext()` - Returns whether there is another item in the sequence.

`myIterator.getNext()` - Returns the next item in the sequence and advances iteration forward.

## Error Classes

`Error` - Base class for other error classes.

`Error.new(message: null)` - Creates a new error with the given message.

`NameError` - Thrown when trying to resolve a missing variable or member name.

`NotImplementedError` - Thrown when trying to invoke an abstract method.

`TypeError` - Thrown when an input has the wrong type.

`ClassError` - Thrown when an input has the wrong class.

`ValueError` - Thrown when an input has an inappropriate value, but the correct type (and class, if applicable).

`StateError` - Thrown when it is not appropriate to perform a particular operation at the current time.

`ResourceError` - Thrown when an external resource, such as storage or a network interface, fails to perform the desired operation.

## Math Class

`Math.floor(num)` - Rounds the number down to the nearest integer.

`Math.rand()` - Generates a random number between 0 and 1.

## BreadText Class Methods

`BreadText.pressKey(key)` - Simulates a key press in the editor.

`BreadText.getMode()` - Returns the editor operation mode.

`BreadText.setMode(mode)` - Sets the editor operation mode.

`BreadText.getSelectionContents()` - Returns the text highlighted in the buffer.

`BreadText.getLineCount()` - Returns the total number of lines in the buffer.

`BreadText.getLineContents(index)` - Returns the text in the given line.

`BreadText.getCursorCharIndex()` - Returns the selected character index within the cursor's line.

`BreadText.getCursorLineIndex()` - Returns the cursor's line index within the entire buffer.

`BreadText.setCursorPos(charIndex, lineIndex)` - Moves the cursor to the given position.

`BreadText.runCommand(commandName, argList)` - Runs a command which the user could otherwise invoke by pressing the forward-slash key. Returns an output value or null.

`BreadText.notifyUser(text)` - Shows a message in the status bar.

`BreadText.promptKey()` - Waits for the user to press a key. Returns the pressed key.

`BreadText.promptChar()` - Waits for the user to press a key. Returns the corresponding character if the user pressed a character key. Returns null otherwise.

`BreadText.bindKey(key, callback)` - Causes the callback to be invoked when the user presses the given key. The callback function should return a boolean indicating whether to override the default action.

`BreadText.mapKey(oldKey, newKey, mode)` - Maps one key to another in the given mode.

`BreadText.bindCommand(name, callback)` - Sets up a command which the user can invoke by pressing the forward-slash key. The callback function should accept a list of arguments. The callback function may return a value to be consumed by `runCommand`.

## BreadText Class Constants

`BreadText.KEY_ESCAPE`  
`BreadText.KEY_LEFT`  
`BreadText.KEY_RIGHT`  
`BreadText.KEY_UP`  
`BreadText.KEY_DOWN`  
`BreadText.KEY_SPACE`  
`BreadText.KEY_NEWLINE`  
`BreadText.KEY_BACKSPACE`  
`BreadText.KEY_TAB`  
`BreadText.KEY_BACKTAB`

`BreadText.MODE_COMMAND`  
`BreadText.MODE_TEXT_ENTRY`  
`BreadText.MODE_TEXT_REPLACE`  
`BreadText.MODE_HIGHLIGHT_CHARACTER`  
`BreadText.MODE_HIGHLIGHT_STATIC`  
`BreadText.MODE_HIGHLIGHT_LINE`


