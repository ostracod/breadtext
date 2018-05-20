
# Values and Operators

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
* Type
* Class
* Object

Boolean values are represented using numbers. Zero is false, while non-zero numbers are true.

ASCII characters may be represented as numbers or buffers. Non-ASCII characters must be represented as buffers.

## Value Literals

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
