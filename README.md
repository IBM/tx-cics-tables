# tx-cics-tables
TXSeries CICS Data Tables prototype

This package is useful where you have tables of data which change infrequently,
and you need high-performance access to the data by key. It was written for
IBM TXSeries CICS, to offer function similar to mainframe CICS data tables,
which are described here
https://www.ibm.com/support/knowledgecenter/en/SSGMCP_5.1.0/com.ibm.cics.ts.applicationprogramming.doc/topics/dfhp3mm.html
but it may also be useful in applications which are not run under CICS.

It was written for 'big endian' systems and tested on AIX; it would require
some rework to run on a 'little endian' system.

First, build the code. There is a Makefile in directory 'src/islarge/src' which 
will build the library, libislarge.a , using the OSF 'make' process. If you 
have a different 'make', you should rewrite this Makefile appropriately.
A suitable OSF 'make' is available here https://github.com/IBM/ode .

Next, write an application which will load data into your tables. There are sample
applications for loading tables in the 'src/islarge/src' directory, but you will need to
revise these for your application. When you run the table load application, it will
create files containing the tables. This application can be written in C or C++.
Files for the tables are written into the directory named by TABLE_DIRECTORY
(see file src/islarge/include/table_int.h ) which by default is /var/pac_tables/
but may be changed if wanted. When you link this application, specify '-l islarge'.

Next, write the application which will use the data in the tables. This may be a
transaction program for TXSeries CICS, or it may be some other application. This
package supports applications written in C, C++, and COBOL. A sample application
showing how to access data in tables is in src/islarge/src/use_table.c . When you
link this application, specify '-l islarge'.

To run the application and access data in the tables, you need to copy the files for
the table onto all nodes where the application may run. The files should go into the
directory named by TABLE_DIRECTORY as above.

To change the data in the tables, run your application which loads the tables again
with the new data, and copy the table files to the nodes where the application which
accesses the tables runs. Then stop and restart the application which accesses the
tables.


