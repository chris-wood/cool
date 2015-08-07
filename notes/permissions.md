Having a model of the program’s calling structure and the relationships between the various Permission objects, SARA identifies, within the call graph, invocations of the access-rights-related APIs: namely checkPermission and doPrivileged. At a given call site invoking checkPermission, SARA retrieves a conservative approximation of the checked Permissions as the points-to set of the argument.

From here SARA can simulate the stack inspection mechanism by propagating the permission requirement up the stack.

To account for Subjects, SARA next traverses the call graph in order to identify doAs and doAsPrivileged calls, wherein the first parameter is of type Subject. Again thanks to points-to information, that parameter is resolved into one or more abstract Subject instances. SARA then consults the points-to graph per each of the instances.


