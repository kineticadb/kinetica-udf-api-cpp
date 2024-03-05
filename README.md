<h3 align="center" style="margin:0px">
	<img width="200" src="https://www.kinetica.com/wp-content/uploads/2018/08/kinetica_logo.svg" alt="Kinetica Logo"/>
</h3>
<h5 align="center" style="margin:0px">
	<a href="https://www.kinetica.com/">Website</a>
	|
	<a href="https://docs.kinetica.com/7.2/">Docs</a>
	|
	<a href="https://docs.kinetica.com/7.2/udf/cpp/writing/">UDF API Docs</a>
	|
	<a href="https://join.slack.com/t/kinetica-community/shared_invite/zt-1bt9x3mvr-uMKrXlSDXfy3oU~sKi84qg">Community Slack</a>   
</h5>


# Kinetica C++ UDF API

-  [Overview](#overview)
-  [API](#api)
-  [Example](#example)
-  [UDF Reference Documentation](#udf-reference-documentation)
-  [Support](#support)
-  [Contact Us](#contact-us)


## Overview

This is the 7.2 version of the server-side C++ UDF API for Kinetica.  UDFs
are server-side programs written in this API and then installed, configured, and
initiated through a separate client-side management API.  These two APIs are
independent of each other and do not need to be written in the same language;
e.g., a UDF can be written in the C++ UDF API and managed in SQL (from
Workbench, KiSQL, or other database client).

The source code for this project can be found at
https://github.com/kineticadb/kinetica-udf-api-cpp

For changes to the client-side API, please refer to
[CHANGELOG.md](CHANGELOG.md).


## API

This repository contains the Kinetica C++ UDF API in the `kinetica` directory.

In the `kinetica` directory, are the two files that compose the API, `Proc.hpp`
and `Proc.cpp`.  These can simply be directly included into any C++ UDF project,
as required.  There are no external dependencies.

Note that due to native components this must be compiled on Linux with the same
architecture as the Kinetica servers on which the UDFs will be used and a
compatible glibc version.


## Example

This repository also contains an example project in the `proc-example`
directory, which implements a UDF in the C++ UDF API.

This example copies one or more input tables to an output table. If the
output table exists, it must have compatible columns to the input tables.

To build the example, run the following command in the `proc-example` directory:

    make

This will produce an executable, `proc-example`. This executable can be uploaded
to Kinetica via the GAdmin *UDF* tab by clicking *New* and using the following
parameters:

* **Name**: `proc-example` (or as desired)
* **Command**: `./proc-example`
* **Files**: `proc-example`

Once uploaded, the UDF can be executed via GAdmin.


## UDF Reference Documentation

For information about UDFs in Kinetica, please see the User-Defined Functions
sections of the Kinetica documentation:

* **UDF Concepts**:  https://docs.kinetica.com/7.2/udf_overview/
* **C++ UDF API**:  https://docs.kinetica.com/7.2/udf/cpp/writing/
* **C++ UDF Management API**:  https://docs.kinetica.com/7.2/udf/cpp/running/
* **C++ UDF Examples**:  https://docs.kinetica.com/7.2/udf/cpp/examples/


## Support

For bugs, please submit an
[issue on Github](https://github.com/kineticadb/kinetica-udf-api-cpp/issues).

For support, you can post on
[stackoverflow](https://stackoverflow.com/questions/tagged/kinetica) under the
``kinetica`` tag or
[Slack](https://join.slack.com/t/kinetica-community/shared_invite/zt-1bt9x3mvr-uMKrXlSDXfy3oU~sKi84qg).


## Contact Us

* Ask a question on Slack:
  [Slack](https://join.slack.com/t/kinetica-community/shared_invite/zt-1bt9x3mvr-uMKrXlSDXfy3oU~sKi84qg)
* Follow on GitHub:
  [Follow @kineticadb](https://github.com/kineticadb) 
* Email us:  <support@kinetica.com>
* Visit:  <https://www.kinetica.com/contact/>
