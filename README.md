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

Given data consisting of:

```
C|JULIE|LEW|19360222||F|2591 MCCRACKEN STREET|||MUSKEGON|MI|49441
C|JULIE|LEW|19360222||F|144 CROSS ST.|||LAKEWOOD|NJ|87010
TONIA|SHERYL|DEKKER|19950218||F|914 GRUBB DRIVE|||MESQUITE|TX|75149
KENNETH|F|MARTIN|19781008||M|1 PETIT PL|||ISLAND PARK|NY|11558
KENNETH|F|MARTIN|19781008||M|7100 CESSNA DR|||GREENSBORO|NC|27409
J|DALE|YOUNG|19840306||M|1913 HWY 163|||RICEVILLE|TN|37370
J|DALE|YOUNG|19840306||M|FM ROAD 990|||SIMMS|TX|75574
REBECCA||NIX|19430125||F|3485 S POINT PRAIRIE RD|% JOYCE ZOLLMANN||WENTZVILLE|MO|63385
REBECCA||NIX|19430125||F|1919 E 13TH ST|LABLE & CO||CLEVELAND|OH|44114
REBECCA||NIX|19430125||F|1 ADA PKWY STE 100|||IRVINE|CA|92618
REBECCA||NIX|19430125||F|1391 170TH AVE NW|WILLIAM ZIEGLER GEN PTR||GEORGETOWN|MN|56546
C|B|BATLEY|19990606||M|26564 WESTGATE RD|||NEOLA|IA|51559
C|B|BATLEY|19990606||M|415 W. 6TH STREET|||VANCOUVER|WA|98660
SHONA||ABUGHAZALEH|19890110||F|2204 FAIRWAY DR|||DODGE CITY|KS|67801
SHONA||ABUGHAZALEH|19890110||F|805 W AIRLINE AVE|||GASTONIA|NC|28052
PAMELA|E|SNOW|19470428||F|PO BOX 847|||BERNALILLO|NM|87004
J|TERA|BRITTON|19921103||F|3343 PAUL DAVIS DRIVE|||MARINA|CA|93933
COLLEEN|S|STRANGE|19600411||F|PO BOX 84|||HAGUE|VA|22469
COLLEEN|S|STRANGE|19600411||F|2318 N COVINGTON ST|||WICHITA|KS|67205
```

The output would look like this:

```
jwoods@pecan:trunk$ ./csvprof sample.csv 
Total of 19 records read
    Record Length: 42 (min), 76 (max), 53 (avg) bytes:
    Field[0]:     1 (min),     7 (max),     4 (avg),    19 (count),     0 (empty)
        HAS_ALPHA HAS_UPPER 
    Field[1]:     0 (min),     6 (max),     1 (avg),    19 (count),     6 (empty)
        HAS_ALPHA HAS_UPPER 
    Field[2]:     3 (min),    11 (max),     5 (avg),    19 (count),     0 (empty)
        HAS_ALPHA HAS_UPPER 
    Field[3]:     8 (min),     8 (max),     8 (avg),    19 (count),     0 (empty)
        HAS_DIGIT 
    Field[4]:     0 (min),     0 (max),     0 (avg),    19 (count),    19 (empty)
    Field[5]:     1 (min),     1 (max),     1 (avg),    19 (count),     0 (empty)
        HAS_ALPHA HAS_UPPER 
    Field[6]:     9 (min),    23 (max),    15 (avg),    19 (count),     0 (empty)
        HAS_ALPHA HAS_UPPER HAS_DIGIT HAS_SPACE HAS_PUNCT 
    Field[7]:     0 (min),    23 (max),     2 (avg),    19 (count),    16 (empty)
        HAS_ALPHA HAS_UPPER HAS_SPACE HAS_PUNCT 
    Field[8]:     0 (min),     0 (max),     0 (avg),    19 (count),    19 (empty)
    Field[9]:     5 (min),    11 (max),     8 (avg),    19 (count),     0 (empty)
        HAS_ALPHA HAS_UPPER HAS_SPACE 
    Field[10]:     2 (min),     2 (max),     2 (avg),    19 (count),     0 (empty)
        HAS_ALPHA HAS_UPPER 
    Field[11]:     5 (min),     5 (max),     5 (avg),    19 (count),     0 (empty)
        HAS_DIGIT 
```

From this output, we can tell:
* fields 0, 2, 3, 5, 6, 9, 10, and 11 are never empty
* fields 4 and 8 are always empty
* There are no lower case alphabetic characters in the data in any field (may
  be important if checking that data is properly standardized).
* fields 3 and 11 are always numeric
* fields 3, 5, and 10 are fixed length.
* fields 6, 7, and 9 contain whitespace (representing multiple tokens or words).
* All other fields contain a single token


If you know what your input data is supposed to contain, you can very quickly confirm that the content
matches your specification.  If you're doing discovery (perhaps for ETL), this will tell you how to
configure your input fields.

ToDo:  this program captures a lot more information than is currently
being output.  At some point, it will produce graphical representations
of byte values for each offset within each field.  These graphical
representations should allow differences in fields, record structure, or
files to be detected quickly.
