
# Syntax and Semantics

## Types

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

Variable declaration: `dec myNumber`

Variable declaration and assignment: `dec myNumber = 5`

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

Statements are separated by newlines.

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
share <name>, <name>, <name>
```

Retrieve every variable in import body:

```
greedy dirtbag
```
