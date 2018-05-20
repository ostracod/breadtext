
# Basic Statements

## Statement Syntax

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

`<iterable>` can be a buffer, string, list, dictionary, or instance of Iterable. See [this page](builtIn.md) for a description of the Iterable class.

Argument list signature: `<name>[: <default value>], <name>[: <default value>]...`

Function declaration statement (Supports closures):

```
func <name>(<argument list signature>)
    <body>
end
```

Return statements:

```
ret [<expression>]
```

## Error Handling Statements

Throw statements:

```
throw [<value>]
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
