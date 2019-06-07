
# BreadText Scripting Language

## Set-up

Place your BreadText script in `~/.breadtextrc.btsl`. It will be evaluated whenever you start up BreadText.

## Syntax and Semantics

Statements are separated by newlines.

Variables are dynamically typed.

Heap values are garbage-collected using mark-and-sweep.

Data types:

* Null
* Number (double-precision float)
* String (mutable, dynamic length)
* List (mutable, dynamic length)
* Function

Null literal: `null`

Number literal: `1.23`

Boolean literals (actually numbers): `true`, `false`

Character literal (actually a number): `'a'`

String literal: `"Hello"`

List literal: `[1, 2, 3]`

Comment: `# comment`

## Operators

Assigment: `=`

Arithmetic operators: `+`, `-`, `*`, `/`, `%`  
Arithmetic assignment operators: `+=`, `-=`, `*=`, `/=`, `%=`

Boolean operators: `&&`, `||`, `^^`, `!`  
Boolean assignment operators: `&&=`, `||=`, `^^=`

Bitwise operators: `&`, `|`, `^`, `~`, `>>`, `<<`  
Bitwise assignment operators: `&=`, `|=`, `^=`, `>>=`, `<<=`

Value comparison: `>`, `<`, `==`, `!=`, `>=`, `<=`

Identity comparison: `===`, `!==`

Increment and decrement: `++`, `--`

List member access: `myList[myIndex]`

Function invocation: `myFunc(myValue1, myValue2, myValue3...)`

## Statements

Expression statement:

```
<expression>
```

Variable declaration:

```
dec <name>
```

Variable declaration and assignment:

```
dec <name> = <expression>
```

If statement:

```
if <condition>
    <body>
else if <condition>
    <body>
else
    <body>
end
```

While statement:

```
while <condition>
    <body>
end
```

Break statement:

```
break
```

Continue statement:

```
continue
```

Function declaration statement:

```
func <name>(<name>, <name>, <name>...)
    <body>
end
```

Return with value statement:

```
ret <expression>
```

Return without value statement:

```
ret
```

Import statement:

```
import <path>
    <body>
end
```

Retrieve specific variables in import body:

```
share <name>, <name>, <name>...
```

## General-purpose Built-in Functions

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

`push(sequence, item)` - Appends a value to the end of the list or string.

`rem(sequence, index)` - Removes a value from the list or string.

`getTimestamp()` - Returns the current Unix timestamp.

## BreadText-specific Built-in Functions

`pressKey(key)` - Simulates a key press in the editor.

`pressKeys(sequence)` - Simulates multiple key presses.

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

## Built-in Constants

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

## Example Scripts

Primality test:

```
# Returns true iff the given number is prime.
func isPrime(number)
    dec factor = 2
    while factor < number
        if number % factor == 0
            ret false
        end
        factor++
    end
    ret true
end

# Try to convert the given text into a number.
# Then notify whether the number is prime.
# Return null or a boolean indicating primality.
func notifyPrimality(text)
    dec number = num(text)
    if number === null
        notifyUser("Text is not a number.")
        ret null
    end
    if isPrime(number)
        notifyUser("Number is prime.")
        ret true
    else
        notifyUser("Number is NOT prime.")
        ret false
    end
end

# Tell the user whether the highlighted number is prime.
func checkSelectionPrimality()
    dec mode = getMode()
    if mode != MODE_HIGHLIGHT_STATIC && mode != MODE_HIGHLIGHT_CHARACTER
        ret false
    end
    dec text = getSelectionContents()
    notifyPrimality(text)
    # Return true to override the default key action.
    ret true
end

# Bind the '7' key to checkSelectionPrimality.
# Invoke by pressing the '7' key.
bindKey('7', checkSelectionPrimality)

# Callback for primality command.
func primalityCommand(argList)
    if len(argList) != 1
        notifyUser("Error: Wrong number of arguments.")
        ret null
    end
    ret notifyPrimality(argList[0])
end

# Set up the primality command.
# Invoke by typing "/isPrime <number>".
bindCommand("isPrime", primalityCommand)
```

For more example scripts, please see [this directory](exampleScripts).



