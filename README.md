This utility generates statistics for simple delimited data files.  It
does not (yet) handle quoted data or data fields which themselves contain
delimiters (eg, quoted CSV data).

To build
* you must have libcsv3 available.  Download it here: https://github.com/robertpostill/libcsv
* In the Makefile, update the location of libcsv (libraries, headers) to reflect where the library was installed on your host.
* run "make".

To run:
    csvprof [-d <delimiter>] [-h <hasHeader>] [-v] [inputFile]

        delimiter - the field delimiter (a single character).  If not given,
                    default is a pipe character ('|').

        hasHeader - must be [0|1].  If 1, the first row will be used as a
                    header (contains field names).  If 0, fields will
                    simply be enumerated.  Default is 0.

        verbose   - if given, verbose output will be generated.  This
                    consists of some additional run time statistics, but
                    does not generate additional analytical information.

        inputFile - the name of the file to be analyzed.  If not given,
                   standard input will be used.

Output:


ToDo:  this program captures a lot more information than is currently
being output.  At some point, it will produce graphical representations
of byte values for each offset within each field.  These graphical
representations should allow differences in fields, record structure, or
files to be detected quickly.
