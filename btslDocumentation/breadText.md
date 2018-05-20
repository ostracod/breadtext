
# BreadText Class

## BreadText Class Methods

`BreadText.pressKey(key)` - Simulates a key press in the editor.

`BreadText.mode [= value]` - The current editor operation mode.

`BreadText.selectionContents` - Returns the text highlighted in the buffer.

`BreadText.lineCount` - Returns the total number of lines in the buffer.

`BreadText.getLineContents(index)` - Returns the text in the given line.

`BreadText.cursorCharIndex` - Returns the selected character index within the cursor's line.

`BreadText.cursorLineIndex` - Returns the cursor's line index within the entire buffer.

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
