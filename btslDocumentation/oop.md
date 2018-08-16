
# Object-Oriented Functionality

## OOP Statement Syntax

Class declaration:

```
class <name> extends <class>, <class>, <class>...
    <body>
end
```

Superclass chain: `<class> -> <class> -> <class> -> ...`

Member variable declaration and initialization statements:

```
[static] mem <name> [= <expression>] [overrides <superclass chain>]
```

Method declaration statements:

```
[static] method <name>(<argument list signature>) [overrides <superclass chain>]
    <body>
end
```

```
[static] getter <name>() [overrides <superclass chain>]
    <body>
end
```

```
[static] setter <name>(<name>) [overrides <superclass chain>]
    <body>
end
```

Custom binding statements:

```
[static] binding <superclass> through <superclass chain>
```

## Object-Oriented Features

When a function is bound to a value, the keyword `this` in the function body will refer to the bound value.

A method is actually a member variable whose value is a function. The function is bound to the class or an instance of the class.

All classes are implicitly subclasses of the `Object` class. See [this page](builtIn.md) for the list of members associated with `Object`.

Every instance of an object is associated with two classes: a binding class, and an overriding class. An object can only be bound to its direct superclasses (unless a custom binding is provided).

An object stores members for its overriding class and all superclasses of the overriding class. Members can only be accessed from the object's binding class.

The overriding class defines how to override members of an object.

A class is also associated with a binding class and an overriding class. Class members behave in an analagous fashion to object members.
