#include <stdio.h>
#include <stdint.h>
#include <stdlib.h>
#include <string.h>
#include <ctype.h>
#include <errno.h>

#include "libcsv/csv.h"

#define INITIAL_FIELDS  16
#define INITIAL_OFFSETS 32

#if 0
#define DEBUG 1
#endif

struct Globals
{
    char delim;
    int  hasHeader;
};

struct Globals glb;

/* Data about what we've found in each offset location */
struct Offset
{
    uint8_t minVal;    /* MIN (byte) value found for this position */
    uint8_t maxVal;    /* MAX (byte) value found for this position */
    int64_t count;     /* number of times we've updated this offset */

    int64_t vals[256]; /* a counter of times each value has been observed */
};

#define F_FL_NONASCII (0x0001)
#define F_FL_NONPRINT (0x0002)
#define F_FL_HASALPHA (0x0004)
#define F_FL_HASLOWER (0x0008)
#define F_FL_HASUPPER (0x0010)
#define F_FL_HASDIGIT (0x0020)
#define F_FL_HASSPACE (0x0040)

struct Field
{
    char *name;
    uint32_t flags;

    uint64_t minLen;   /* the MIN (byte) length found for this field */
    uint64_t maxLen;   /* the MAX (byte) length found for this field */
    uint64_t totLen;   /* the TOTAL number of bytes in this field */
    uint64_t numEmpty;

    struct Offset **offsets;  /* data about what was found in each offset */
    uint64_t count;           /* number of times we've updated this field */
    uint64_t maxOffsets;      /* max number of offs which might be stored in
                                 the offsets[] array without having to
                                 resize the array. */
};

struct Record
{
    int rown;
    int fieldn;
    int reclen;

    uint64_t minLen;   /* min observed length (bytes) of a record */
    uint64_t maxLen;   /* max observed length (bytes) of a record */
    uint64_t count;    /* number of records read */
    uint64_t totLen;   /* total length (bytes) of all records combined */

    struct Field **fields;
    int      maxFields;
};

static inline struct Offset *new_offset()
{
    struct Offset *off;

    off = malloc(sizeof(struct Offset));
    if (off == (struct Offset *) NULL)
    {
        fprintf(stderr, "ERROR: malloc() failed: %s (%s:%d)\n",
                strerror(errno), __FILE__, __LINE__);
        return (struct Offset *) NULL;
    }

    memset(off, 0x00, sizeof(*off));
    off->minVal = (uint8_t) -1;
    off->maxVal = 0;

    return off;
}

/* allocate a new field */
static inline struct Field *new_field()
{
    struct Field *fld;
    int i;

    fld = malloc(sizeof(struct Field));
    if (fld == (struct Field *) NULL)
    {
        fprintf(stderr, "ERROR: malloc() failed: %s (%s:%d)\n",
                strerror(errno), __FILE__, __LINE__);
        exit(1);
    }

    memset(fld, 0x00, sizeof(*fld));
    fld->minLen = (uint64_t) -1;

    fld->offsets = (struct Offset **)
        malloc(sizeof(struct Offset *) * INITIAL_OFFSETS);

    if (fld->offsets == (struct Offset **) NULL)
    {
        free(fld);
        fprintf(stderr, "ERROR: malloc() failed: %s (%s:%d)\n",
                strerror(errno), __FILE__, __LINE__);
        exit(1);
    }

    memset(fld->offsets, 0x00, sizeof(struct Offset *) * INITIAL_OFFSETS);

    for (i = 0; i < INITIAL_OFFSETS; i++)
    {
        fld->offsets[i] = new_offset();
        if (fld->offsets[i] == (struct Offset *) NULL)
        {
            fprintf(stderr, "ERROR: malloc() failed: %s (%s:%d)\n",
                    strerror(errno), __FILE__, __LINE__);
            exit(1);
        }
    }

    fld->maxOffsets = i;

    return fld;
}

/* allocate a new record stats block (one per process instance) */
static inline struct Record *new_record()
{
    struct Record *rec;
    int f;

    rec = malloc(sizeof(struct Record));
    if (rec == (struct Record *) NULL)
    {
        fprintf(stderr, "ERROR: malloc() failed: %s (%s:%d)\n",
                strerror(errno), __FILE__, __LINE__);
        exit(1);
    }

    memset(rec, 0x00, sizeof(*rec));

    rec->minLen = (uint64_t) -1;
    rec->maxLen = 0;
    rec->count = 0;
    rec->totLen = 0;

    rec->fields = (struct Field **)
        malloc(sizeof(struct Field *) * INITIAL_FIELDS);
    if (rec->fields == (struct Field **) NULL)
    {
        fprintf(stderr, "ERROR: malloc() failed: %s (%s:%d)\n",
                strerror(errno), __FILE__, __LINE__);
        exit(1);
    }

    memset(rec->fields, 0x00, sizeof(struct Field *) * INITIAL_FIELDS);

    for (f = 0; f < INITIAL_FIELDS; f++)
        rec->fields[f] = new_field();    /* does not return on error */

    rec->maxFields = f;
    return rec;
}

static inline void del_offset(struct Offset *off)
{
    free(off);
    return;
}

static inline void del_field(struct Field *field)
{
    int off;

    if (field->name != (char *) NULL) free(field->name);

    for (off = 0; off < field->maxOffsets; off++)
        del_offset(field->offsets[off]);

    free(field->offsets);
    free(field);
    return;
}

static inline void del_record(struct Record *rec)
{
    int fno;

    for (fno = 0; fno < rec->maxFields; fno++)
        del_field(rec->fields[fno]);

    free(rec->fields);
    free(rec);
    return;
}

static void resize_offsets_array(struct Field *field)
{
    struct Offset **old_offsets;
    struct Offset **new_offsets;
    int old_maxOffsets;
    int new_maxOffsets;
    int off;
    
    old_maxOffsets = field->maxOffsets;
    old_offsets = field->offsets;
    new_maxOffsets = old_maxOffsets * 2;

#ifdef DEBUG
    printf("  ** increasing offsets for field %d from %d to %d\n",
           f, old_maxOffsets, new_maxOffsets);
#endif

    new_offsets = (struct Offset **)
         malloc(sizeof(struct Offset *) * new_maxOffsets);

    if (new_offsets == (struct Offset **) NULL)
    {
        fprintf(stderr, "ERROR: malloc() failed: %s (%s:%d)\n",
                strerror(errno), __FILE__, __LINE__);
        exit(1);
    }

    /* Copy the old offsets... */
    memcpy(new_offsets, old_offsets,
           sizeof(struct Offset **) * old_maxOffsets);
        
    /* ...and initialize the new */
    for (off = old_maxOffsets; off < new_maxOffsets; off++)
        new_offsets[off] = new_offset();

    field->offsets = new_offsets;
    field->maxOffsets = new_maxOffsets;
    free(old_offsets);

    return;
}

static void update_offset(struct Record *rec, int f, int o, uint8_t b)
{
    struct Field *field;
    struct Offset *offset;

    field = rec->fields[f];
    offset = field->offsets[o];

    if (b < offset->minVal) offset->minVal = b;
    if (b > offset->maxVal) offset->maxVal = b;
    offset->vals[b]++;
    offset->count++;

    /* TODO - we might optimize these checks for the more common
              cases first.  */

    if (! isascii(b)) field->flags |= F_FL_NONASCII;
    else
    {
        if (! isprint(b)) field->flags |= F_FL_NONPRINT;
        else
        {
            if (isalpha(b))
            {
                field->flags |= F_FL_HASALPHA;

                if (islower(b)) field->flags |= F_FL_HASLOWER;
                if (isupper(b)) field->flags |= F_FL_HASUPPER;
            }
            else if (isdigit(b))   field->flags |= F_FL_HASDIGIT;
            else if (isspace(b))   field->flags |= F_FL_HASSPACE;
        }
    }

    return;
}

static void resize_fields_array(struct Record *rec)
{
    struct Field **old_fields;
    struct Field **new_fields;
    int old_maxFields;
    int new_maxFields;
    int fno;

    old_fields = rec->fields;
    old_maxFields = rec->maxFields;
    new_maxFields = rec->maxFields * 2;

#ifdef DEBUG
    printf("** increasing fields from %d to %d\n",
           old_maxFields, new_maxFields);
#endif

    new_fields = (struct Field **)
        malloc(sizeof(struct Field *) * new_maxFields);

    if (new_fields == (struct Field **) NULL)
    {
        fprintf(stderr, "ERROR: malloc() failed: %s (%s:%d)\n",
                strerror(errno), __FILE__, __LINE__);
        exit(1);
    }

    /* copy old fields into new array */
    memcpy(new_fields, old_fields,
           sizeof(struct Field **) * old_maxFields);

    /* allocate new fields */
    for (fno = old_maxFields; fno < new_maxFields; fno++)
    {
        new_fields[fno] = new_field();    
        if (new_fields[fno] == (struct Field *) NULL)
        {
            fprintf(stderr, "ERROR: malloc() failed: %s (%s:%d)\n",
                    strerror(errno), __FILE__, __LINE__);
            exit(1);
        }
    }

    rec->fields = new_fields;
    rec->maxFields = new_maxFields;
    free(old_fields);

    return;
}

static inline void update_field(struct Record *rec, int fno, int len)
{
    struct Field *field;


    field = rec->fields[fno];

    if (len < field->minLen) field->minLen = len;
    if (len > field->maxLen) field->maxLen = len;
    if (len == 0) field->numEmpty++;
    field->totLen += len;
    field->count++;

    return;
}

static inline void update_record(struct Record *rec, int len)
{
    if (len < rec->minLen) rec->minLen = len;
    if (len > rec->maxLen) rec->maxLen = len;
    rec->totLen += len;
    rec->count++;

    if (rec->count % 1000 == 0)
        fprintf(stdout, "%d records completed\n", (int) rec->count);

    return;
}

static void do_report(struct Record *rec)
{
    struct Field *fld;
    int fno;

    printf("Total of %ld records read\n", rec->count);
    printf("    Record Length: %lu (min), %lu (max), %lu (avg) bytes:\n",
           rec->minLen, rec->maxLen, (rec->totLen / rec->count));
    
    /* if we hit a field which has never been updated (count == 0), we know
       we can stop.  Even an empty field (length == 0) updates the counter. */
    for (fno = 0;
         fno < rec->maxFields && rec->fields[fno]->count > 0;
         fno++)
    {
        fld = rec->fields[fno];
        if (fld->name != (char *) NULL)
            printf("    Field[%d]: %s\n        ", fno, fld->name);
        else
            printf("    Field[%d]: ", fno);

        printf("%5lu (min), %5lu (max), %5lu (avg) bytes, %5lu (empty)\n",
                fld->minLen, fld->maxLen,
                (fld->totLen / fld->count),
                fld->numEmpty);

        int out = 0;
        if ((fld->flags & F_FL_NONASCII) != 0)
        {
            if (out == 0) printf("        ");
            printf("**NONASCII** ");
            out = 1;
        }

        if ((fld->flags & F_FL_NONPRINT) != 0)
        {
            if (out == 0) printf("        ");
            printf("**NONPRINT** ");
            out = 1;
        }

        if ((fld->flags & F_FL_HASALPHA) != 0)
        {
            if (out == 0) printf("        ");
            printf("HAS_ALPHA ");
            out = 1;
        }

        if ((fld->flags & F_FL_HASLOWER) != 0)
        {
            if (out == 0) printf("        ");
            printf("HAS_LOWER ");
            out = 1;
        }

        if ((fld->flags & F_FL_HASUPPER) != 0)
        {
            if (out == 0) printf("        ");
            printf("HAS_UPPER ");
            out = 1;
        }

        if ((fld->flags & F_FL_HASDIGIT) != 0)
        {
            if (out == 0) printf("        ");
            printf("HAS_DIGIT ");
            out = 1;
        }

        if ((fld->flags & F_FL_HASSPACE) != 0)
        {
            if (out == 0) printf("        ");
            printf("HAS_SPACE ");
            out = 1;
        }

        if (out != 0) printf("\n");
    }

    return;    
}

void endoffield(void *s, size_t len, void *data)
{
    struct Record *rec;
    int fno;
    struct Field *field;
    uint8_t *bytes;
    int off;

    rec = (struct Record *) data;
    bytes = (uint8_t *) s;

    fno = rec->fieldn;

    /* There is no way to know how many fields are in a record before we hit
       the end of the record, so we'll need to check that we've not hit the
       end of the fields array prior to updating each field.  We may need to
       resize the fields array if we've grown beyond what was previously
       allocated. */
    if (fno >= rec->maxFields)
         resize_fields_array(rec);   /* does not return on error */

    field = rec->fields[fno];
    if (glb.hasHeader != 0 && rec->rown == 0)
    {
        /* processing header row, copy field name */
        field->name = (char *) malloc(len + 1);
        if (field->name == (char *) NULL)
        {
            fprintf(stderr, "ERROR: malloc() failed: %s (%s:%d)\n",
                    strerror(errno), __FILE__, __LINE__);
            exit(1);
        }
        
        memcpy(field->name, s, len);
        field->name[len] = '\0';

#ifdef DEBUG
        printf("[%d] %s\n", fno, field->name);
#endif
    }
    else
    {
        update_field(rec, fno, len);
    
        for (off = 0; off < len; off++)
        {
            /* we may need to resize the offsets array if we've grown
               beyond what was previously allocated. */

            /* TODO - we know the length of the field (as an input
                      parameter), so we don't need to do this check for
                      every offset! */

            if (off >= field->maxOffsets) resize_offsets_array(field);

            update_offset(rec, fno, off, bytes[off]);
        }
    }

    rec->reclen += len;
    rec->fieldn++;
    return;
}

void endofrec(int c, void *data)
{
    struct Record *rec = (struct Record *) data;

    update_record(rec, rec->reclen);

    rec->rown++;
    rec->fieldn = 0;
    rec->reclen = 0;
    return;
}

void Initialize(int argc, char **argv)
{
    memset(&glb, 0x00, sizeof(glb));
    /* glb.delim = '\t'; */
    glb.delim = '|';
    /* glb.hasHeader = 1; */
    glb.hasHeader = 0;

    /* TODO - complete */

    return;
}

int main(int argc, char **argv)
{
    FILE *fp;
    struct csv_parser p;
    char buf[8192];
    size_t bytes_read;
    struct Record *rec;

    Initialize(argc, argv);

    rec = new_record();   /* does  not return on error */

    if (csv_init(&p, 0) != 0) exit(EXIT_FAILURE);
    csv_set_delim(&p, glb.delim);

    fp = fopen(argv[1], "rb");
    if (fp == (FILE *) NULL)
    {
        fprintf(stderr, "ERROR: fopen(%s) failed: %s (%s:%d)\n",
                argv[1], strerror(errno), __FILE__, __LINE__);
         exit(1);
    }

    while ((bytes_read = fread(buf, 1, sizeof(buf), fp)) > 0)
    {
        if (csv_parse(&p, buf, bytes_read,
                      endoffield, endofrec, rec) != bytes_read)
        {
            fprintf(stderr, "Error while parsing file: %s\n",
                    csv_strerror(csv_error(&p)) );
            exit(EXIT_FAILURE);
        }
    }

    csv_fini(&p, endoffield, endofrec, rec);
    fclose(fp);
    csv_free(&p);

    do_report(rec);
    del_record(rec);

    exit(EXIT_SUCCESS);
}
