
# Built-In Values

## General-Purpose Functions

`isNum(value)` - Returns true iff the value is a number.

`isStr(value)` - Returns true iff the value is a string.

`isList(value)` - Returns true iff the value is a list.

`isFunc(value)` - Returns true iff the value is a function.

`copy(value)` - Returns a shallow copy of the value.

`str(number)` - Converts a number into a string.

`num(text)` - Converts a string into a number. Returns null if the text cannot be converted.

`floor(num)` - Rounds the number down to the nearest integer.

`rand()` - Generates a random number between 0 and 1.

`len(sequence)` - Returns the length of a list or string.

`ins(sequence, index, item)` - Inserts a value into the list or string.

`rem(sequence, index)` - Removes a value from the list or string.

## BreadText-Specific Functions

`pressKey(key)` - Simulates a key press in the editor.

`getMode()` - Returns the editor operation mode.

`setMode(mode)` - Sets the editor operation mode.

`getSelectionContents()` - Returns the text highlighted in the buffer.

`getLineCount()` - Returns the total number of lines in the buffer.

`getLineContents(index)` - Returns the text in the given line.

`getCursorCharIndex()` - Returns the selected character index within the cursor's line.

`getCursorLineIndex()` - Returns the cursor's line index within the entire buffer.

`setCursorPos(charIndex, lineIndex)` - Moves the cursor to the given position.

`runCommand(commandName, argList)` - Runs a command which the user could otherwise invoke by pressing the forward-slash key. Returns an output value or null.

`notifyUser(text)` - Shows a message in the status bar.

`promptKey()` - Waits for the user to press a key. Returns the pressed key.

`promptChar()` - Waits for the user to press a key. Returns the corresponding character if the user pressed a character key. Returns null otherwise.

`bindKey(key, callback)` - Causes the callback to be invoked when the user presses the given key. The callback function should return a boolean indicating whether to override the default action.

`mapKey(oldKey, newKey, mode)` - Maps one key to another in the given mode.

`bindCommand(name, callback)` - Sets up a command which the user can invoke by pressing the forward-slash key. The callback function should accept a list of arguments. The callback function may return a value to be consumed by `runCommand`.

## Constants

`KEY_ESCAPE`  
`KEY_LEFT`  
`KEY_RIGHT`  
`KEY_UP`  
`KEY_DOWN`  
`KEY_SPACE`  
`KEY_NEWLINE`  
`KEY_BACKSPACE`  
`KEY_TAB`  
`KEY_BACKTAB`

`MODE_COMMAND`  
`MODE_TEXT_ENTRY`  
`MODE_TEXT_REPLACE`  
`MODE_HIGHLIGHT_CHARACTER`  
`MODE_HIGHLIGHT_STATIC`  
`MODE_HIGHLIGHT_LINE`


