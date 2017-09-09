
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

Variable declaration and assignment: `myNumber = 5`

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
ret <value>
```

Return without value statement:

```
ret
```

Import statement:

```
import <path>
```

## General-purpose Built-in Functions

`isNum(value)`  
`isStr(value)`  
`isList(value)`  
`isFunc(value)`  
`copy(value)` - Shallow copy.  
`str(number)`  
`num(text)`  - Returns null if the text cannot be converted.  
`floor(num)`  
`len(list)`  
`ins(sequence, index, item)` - Inserts a value into the list or string.  
`rem(sequence, index)` - Removes a value from the list or string.

## BreadText-specific Built-in Functions

`pressKey(num)`  
`getMode()`  
`setMode(mode)`  
`getSelection()` - Returns the string highlighted in the editor.  
`getLineCount()`  
`getLine(index)` - Returns the contents of the given line.  
`getCursorCharIndex()` - Returns the selected character index within the cursor's line.  
`getCursorLineIndex()` - Returns the cursor's line index within the entire buffer.  
`setCursorPos(charIndex, lineIndex)`  
`runCommand(commandName, argList)` - Returns an output value or null.  
`notifyUser(text)` - Shows a message in the status bar.  
`promptKey()`  
`promptCharacter()` - Returns null if the user pressed a non-character key.  
`bindKey(num, callback)` - The callback function should return a boolean indicating whether to override the default action.  
`bindCommand(name, callback)` - The callback function should accept a list of arguments. The callback function may return a value to be consumed by `runCommand`.

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
    factor = 2
    while factor < number
        if number % factor == 0
            ret false
        factor++
    end
    ret true
end

# Try to convert the given text into a number.
# Then notify whether the number is prime.
# Return null or a boolean indicating primality.
func notifyPrimality(text)
    number = num(text)
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
    text = getSelection()
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





