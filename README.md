Kinetica C++ API
================

This project contains the source code of the Kinetica UDF C++ API and an example
project.

The documentation can be found at http://www.kinetica.com/docs/6.2/index.html.
UDF-specific documentation can be found at:

*   http://www.kinetica.com/docs/concepts/index.html#user-defined-functions


proc-api
--------

The API source code is in the kinetica directory. It consists of two files,
Proc.cpp and Proc.hpp. These can simply be directly included in any C++
UDF projects as required. There are no external dependencies.

Note that the API (and any projects using it) must be compiled on Linux with
the same architecture as the Kinetica servers on which the UDFs will be used
and a compatible glibc version.


proc-example
------------

The proc-example copies one or more input tables to output tables. If the
output tables exist, they must have compatible columns to the input tables.


To build the example, run the following command in the proc-example directory:

> make


This will produce an executable, proc-example. This executable can be uploaded
to Kinetica via the gadmin UDF tab by clicking "New" and using the following
parameters:

*   Name: proc-example (or as desired)
*   Command: ./proc-example
*   Files: proc-example


Once uploaded, the proc can be executed via gadmin.
