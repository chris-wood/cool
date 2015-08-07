Having a model of the program’s calling structure and the relationships between the various Permission objects, SARA identifies, within the call graph, invocations of the access-rights-related APIs: namely checkPermission and doPrivileged. At a given call site invoking checkPermission, SARA retrieves a conservative approximation of the checked Permissions as the points-to set of the argument.

