
# Non-BreadText Classes

## Object Instance Methods

`myObject.class` - Returns the class of the object.

`myObject.isInstanceOf(class)` - Returns whether the object is an instance of the given class.

`myObject.getSuper(class)` - Returns a version of the object with the given binding and overriding class.

`myObject.super` - Returns a version of the object whose binding and overriding class is the non-Object superclass. If there is not exactly one non-Object superclass, throw an error.

`myObject.compare(item)` - Returns -1, 0, or 1 depending on comparison of values. This method is used for comparison operators, dictionary keys, and the `sort` function.

`myObject.toString()` - Returns a string representation of the object and its values. This method is used by the `str` function.

## Object Class Methods

`Object.new()` - Creates an instance of the class.

`Object.name` - Gets the name of the class.

`Object.isSubclassOf(class)` - Returns whether the class is a subclass of the given class.

`myObject.getSuper(class)` - Returns a version of the class with the given binding and overriding class.

`myObject.super` - Returns a version of the class whose binding and overriding class is the non-Object superclass. If there is not exactly one non-Object superclass, throw an error.

`Object.toString()` - Returns a string representation of the class and its values. This method is used by the `str` function.

## Iterable Class

`myIterable.getIterator()` - Returns an instance of `Iterator`. This method is used in `for` loops.

## Iterator Class

`myIterator.hasNext()` - Returns whether there is another item in the sequence.

`myIterator.getNext()` - Returns the next item in the sequence and advances iteration forward.

## Error Classes

`Error` - Base class for other error classes.

`Error.new(message: null)` - Creates a new error with the given message.

`myError.message` - The message associated with the error.

`NameError` - Thrown when trying to resolve a missing variable or member name.

`NotImplementedError` - Thrown when trying to invoke an abstract method.

`TypeError` - Thrown when an input has the wrong type.

`ClassError` - Thrown when an input has the wrong class.

`ValueError` - Thrown when an input has an inappropriate value, but the correct type (and class, if applicable).

`StateError` - Thrown when it is not appropriate to perform a particular operation at the current time.

`ResourceError` - Thrown when an external resource, such as storage or a network interface, fails to perform the desired operation.

## Math Class

`Math.floor(num)` - Rounds the number down to the nearest integer.

`Math.rand()` - Generates a random number between 0 and 1.



