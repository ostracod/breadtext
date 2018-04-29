
# Syntax and Semantics

## Values and Types

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
* Type
* Class
* Object

Boolean values are represented using numbers. Zero is false, while non-zero numbers are true.

ASCII characters may be represented as numbers or buffers. Non-ASCII characters must be represented as buffers.

Null literal: `null`

Number literal: `1.23`

Boolean literals: `true`, `false`

Character literal: `'a'`

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

Sequence member access: `mySequence[myIndex]`

Function invocation: `myFunc(myValue1, myValue2, myValue3...)`  
Argument labels: `myFunc(myName1: myValue1)`

Object and class member access: `.`  
Direct member access without getters or setters: `..`  
Bind value to class: `myValue{myClass}`

## Basic Statement Syntax

Statements are separated by newlines.

Comment:

```
# <comment>
```

Expression statement:

```
<expression>
```

Variable declaration and initalization statements:

```
dec <name> [= <expression>]
```

## Control Flow Statements

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

Function declaration statement (Supports closures):

```
func <name>(<name>[: <value>], <name>[: <value>], <name>[: <value>]...)
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

## Error Handling Statements

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

## Object-Oriented Statements

Class declaration:

```
class <name> extends <class>, <class>, <class>...
    <body>
end
```

Member variable declaration and initialization statements:

```
[static] [overridable] mem <name> [= <expression>] [overrides <class>]
```

Method declaration statements:

```
[static] [overridable] method <name>(<name>[: <value>], <name>[: <value>]...) [overrides <class>]
    <body>
end
```

## Dependency Statements

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
share <name> [as <name>], <name> [as <name>], <name> [as <name>]...
```

Consume all statement (Retrieve every variable in import body):

```
consume all
```

## Object-Oriented Features

When a function is bound to a value, the keyword `this` in the function body will refer to the bound value.

A method is actually a member variable whose value is a function. The function is bound to the class or an instance of the class.

If an object has a method named `getMyMember`, retrieving `myObject.myMember` will return the value of `myObject.getMyMember()`.

If an object has a method named `setMyMember`, assigning a value to `myObject.myMember` will perform `myObject.setMyMember(value)`.

All classes are implicitly subclasses of the `Object` class. See [this page](builtIn.md) for the list of members associated with `Object`.

Every instance of an object is associated with two classes: a binding class, and an overriding class. An object can only be bound to its overriding class or any superclass of the overriding class.

An object stores members for its overriding class and all superclasses of the overriding class. Members can only be accessed from the object's binding class.

The overriding class defines how to override methods of an object.

A class is also associated with a binding class and an overriding class. Class members behave in an analagous fashion to object members.


