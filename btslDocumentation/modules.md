
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

## Module Version Numbers

All version numbers must have the format `<major>.<minor>.<patch>`.

For each module version, the author must provide documentation describing all "public" variables which are intended to be used externally.

The author must increase the major, minor, or patch value of their module whenever releasing a new version.

The major value should be increased if changes in the module break backward compatibility. Such changes include, but are not limited to:

* Removing or changing public variable names
* Changing the expected functionality of public variables
* Removing arguments from a public function signature
* Adding arguments before the end of a public function signature
* Changing the signature of an overridable public method in any way
* Changing the class which a public member overrides
* Removing or changing a superclass in a public class
* Removing or changing a custom binding in a public class

The minor value should be increased if new features are added (while maintaining backward compatibility).

The patch value should be increased for bug fixes (without adding new features, and while maintaining backward compatibility).

A script cannot use a module if the module's major value is different than requested.

A script can use a module if the module's minor value is greater than requested, but not less (and major value is the same).

A script can use a module regardless of the module's patch value (if major and minor values are the same). However, it is generally recommended to use the latest patch.


