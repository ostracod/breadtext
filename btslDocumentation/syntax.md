
# Syntax and Semantics

## Types

Variables are dynamically typed.

Heap values are garbage-collected using mark-and-sweep and automatic reference counting.

Data types:

* Null
* Number (double-precision float)
* Buffer (mutable byte sequence, dynamic length)
* String (mutable unicode sequence, dynamic length)
* List (mutable value sequence, dynamic length)
* Function
* Dictionary
* Class
* Object

Null literal: `null`

Number literal: `1.23`

Boolean literals (actually numbers): `true`, `false`

Character literal (not actually sure what type this will be): `'a'`

String literal: `"Hello"`

List literal: `[1, 2, 3]`

Dictionary literal: `{"name": "Bob", "age": 47}`

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
Argument labels: `myFunc(myName1: myValue1)`

Member and method access: `.`  
Direct member and method access without getters or setters: `..`

## Statements

Statements are separated by newlines.

Comment:

```
# comment
```

Expression statement:

```
<expression>
```

Variable declaration and initalization statements:

```
dec <name>
```

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

Break and continue statements:

```
break
```

```
continue
```

For statement:

```
for <name> in <iterable>
    <body>
end
```

Throw statements:

```
throw <value>
```

```
throw
```

Try statement:

```
try
    <body>
catch <name>
    <body>
finally
    <body>
end
```

With statement:

```
with <context>
    <body>
end
```

Function declaration statement:

```
func <name>(<name: value?>, <name: value?>, <name: value?>...)
    <body>
end
```

Return statements:

```
ret <expression>
```

```
ret
```

Import statements:

```
import file <path>
    <body>
end
```

```
import module <string>
    <body>
end
```

Share statement (Retrieve specific variables in import body):

```
share <name>, <name>, <name>
```

Consume all statement (Retrieve every variable in import body):

```
consume all
```



