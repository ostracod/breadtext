
# Module Design

## Set-up and Structure

A BTSL application using modules should store a `btslModules.json` file in the top-level directory. This file specifies dependency modules needed by the application:

```
{
    "name": <application name>,
    "version": <application version>,
    "dependencies": [
        {
            "name": <module name>,
            "version": <module version>
        },
        {
            "name": <module name>,
            "version": <module version>
        },
        ...
    ]
}
```

Modules should be placed in a `btslModules` directory, also stored in the top-level directory of the application. Each module version is given its own directory in `btslModules`.

Each module directory must contain a `btslModule.json` file giving information about the module:

```
{
    "name": <module name>,
    "version": <module version>,
    "exportFile": <btsl file path>,
    "dependencies": [
        {
            "name": <module name>,
            "version": <module version>
        },
        {
            "name": <module name>,
            "version": <module version>
        },
        ...
    ]
}
```

When a script includes a module, it is able to access members contained in `exportFile`.


