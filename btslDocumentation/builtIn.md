
# Built-In Values

## Top-Level Functions

`copy(value)` - Returns a shallow copy of the value.

`num(text)` - Converts a string into a number. Returns null if the text cannot be converted.

`buff(value)` - Converts a value into a buffer.

`str(value)` - Converts a value into a string.

`list(value)` - Converts a value into a list.

`type(value)` - Returns the type of the value.

`len(sequence)` - Returns the length of the sequence.

`ins(sequence, index, item)` - Inserts a value into the sequence.

`rem(sequence, index)` - Removes a value from the sequence.

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


