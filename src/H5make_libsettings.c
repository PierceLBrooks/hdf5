/* * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * *
 * Copyright by The HDF Group.                                               *
 * Copyright by the Board of Trustees of the University of Illinois.         *
 * All rights reserved.                                                      *
 *                                                                           *
 * This file is part of HDF5.  The full HDF5 copyright notice, including     *
 * terms governing use, modification, and redistribution, is contained in    *
 * the COPYING file, which can be found at the root of the source code       *
 * distribution tree, or in https://www.hdfgroup.org/licenses.               *
 * If you do not have access to either file, you may request a copy from     *
 * help@hdfgroup.org.                                                        *
 * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * */

/* Keep this declaration near the top of this file */
static const char *FileHeader = "\n\
/* * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * *\n\
 * Copyright by The HDF Group.                                               *\n\
 * Copyright by the Board of Trustees of the University of Illinois.         *\n\
 * All rights reserved.                                                      *\n\
 *                                                                           *\n\
 * This file is part of HDF5.  The full HDF5 copyright notice, including     *\n\
 * terms governing use, modification, and redistribution, is contained in    *\n\
 * the COPYING file, which can be found at the root of the source code       *\n\
 * distribution tree, or in https://www.hdfgroup.org/licenses.               *\n\
 * If you do not have access to either file, you may request a copy from     *\n\
 * help@hdfgroup.org.                                                        *\n\
 * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * *";
/*
 * Purpose:    Generate the H5libsettings.c file from the
 *             libhdf5.settings file.
 *
 *-------------------------------------------------------------------------
 */

#include "H5private.h"

/* Do NOT use HDfprintf in this file as it is not linked with the library,
 * which contains the H5system.c file in which the function is defined.
 */

#define LIBSETTINGSFNAME "libhdf5.settings"

FILE *rawoutstream = NULL;

/*-------------------------------------------------------------------------
 * Function:  insert_libhdf5_settings
 *
 * Purpose:   insert the contents of libhdf5.settings into a file
 *            represented by flibinfo.
 *            Make it an empty string if H5_HAVE_EMBEDDED_LIBINFO is not
 *            defined, i.e., not enabled.
 *
 * Return:    void
 *-------------------------------------------------------------------------
 */
static void
insert_libhdf5_settings(FILE *flibinfo)
{
#ifdef H5_HAVE_EMBEDDED_LIBINFO
    FILE *fsettings; /* for files libhdf5.settings */
    int   inchar;
    int   bol = 0; /* indicates the beginning of a new line */

    if (NULL == (fsettings = HDfopen(LIBSETTINGSFNAME, "r"))) {
        HDperror(LIBSETTINGSFNAME);
        HDexit(EXIT_FAILURE);
    }

    /* Turn off warnings for large arrays. If the library info string is
     * a problem, people can build without the embedded library info.
     */
    HDfprintf(flibinfo, "#include \"H5private.h\"\n");
    HDfprintf(flibinfo, "H5_GCC_DIAG_OFF(\"larger-than=\")\n\n");
    HDfprintf(flibinfo, "H5_CLANG_DIAG_OFF(\"overlength-strings\")\n\n");

    /* Print variable definition and the string. Do not use const or some
     * platforms (AIX?) will have issues.
     */
    HDfprintf(flibinfo, "char H5libhdf5_settings[]=\n");
    bol++;
    while (EOF != (inchar = HDgetc(fsettings))) {
        if (bol) {
            /* Start a new line */
            HDfprintf(flibinfo, "\t\"");
            bol = 0;
        }
        if (inchar == '\n') {
            /* end of a line */
            HDfprintf(flibinfo, "\\n\"\n");
            bol++;
        }
        else
            HDputc(inchar, flibinfo);
    }

    if (HDfeof(fsettings)) {
        /* wrap up */
        if (!bol)
            /* EOF found without a new line */
            HDfprintf(flibinfo, "\\n\"\n");
        HDfprintf(flibinfo, ";\n\n");
    }
    else {
        HDfprintf(stderr, "Read errors encountered with %s\n", LIBSETTINGSFNAME);
        HDexit(EXIT_FAILURE);
    }
    if (0 != HDfclose(fsettings)) {
        HDperror(LIBSETTINGSFNAME);
        HDexit(EXIT_FAILURE);
    }

    /* Re-enable warnings for large arrays */
    HDfprintf(rawoutstream, "H5_GCC_DIAG_ON(\"larger-than=\")\n");
    HDfprintf(rawoutstream, "H5_CLANG_DIAG_OFF(\"overlength-strings\")\n");
#else
    /* Print variable definition and an empty string. Do not use const or some
     * platforms (AIX?) will have issues.
     */
    HDfprintf(flibinfo, "char H5libhdf5_settings[]=\"\";\n");
#endif
} /* insert_libhdf5_settings() */

/*-------------------------------------------------------------------------
 * Function:  make_libinfo
 *
 * Purpose:   Create the embedded library information definition.
 *            This sets up for a potential extension that the declaration
 *            is printed to a file different from stdout.
 *
 * Return:    void
 *-------------------------------------------------------------------------
 */
static void
make_libinfo(void)
{
    /* Print variable definition and then the string as a macro */
    insert_libhdf5_settings(rawoutstream);
}

/*-------------------------------------------------------------------------
 * Function:  print_header
 *
 * Purpose:   Prints the header for the generated file.
 *
 * Return:    void
 *-------------------------------------------------------------------------
 */
static void
print_header(void)
{
    time_t      now = HDtime(NULL);
    struct tm  *tm  = HDlocaltime(&now);
    char        real_name[30];
    char        host_name[256];
    int         i;
    const char *s;
#ifdef H5_HAVE_GETPWUID
    struct passwd *pwd = NULL;
#else
    int pwd      = 1;
#endif
    static const char *month_name[] = {"Jan", "Feb", "Mar", "Apr", "May", "Jun",
                                       "Jul", "Aug", "Sep", "Oct", "Nov", "Dec"};
    static const char *purpose      = "\
This machine-generated source code contains\n\
information about the library build configuration\n";

    /*
     * The real name is the first item from the passwd gecos field.
     */
#ifdef H5_HAVE_GETPWUID
    {
        size_t n;
        char  *comma;

        if ((pwd = HDgetpwuid(HDgetuid()))) {
            if ((comma = HDstrchr(pwd->pw_gecos, ','))) {
                n = MIN(sizeof(real_name) - 1, (unsigned)(comma - pwd->pw_gecos));
                HDstrncpy(real_name, pwd->pw_gecos, n);
                real_name[n] = '\0';
            }
            else {
                HDstrncpy(real_name, pwd->pw_gecos, sizeof(real_name));
                real_name[sizeof(real_name) - 1] = '\0';
            }
        }
        else
            real_name[0] = '\0';
    }
#else
    real_name[0] = '\0';
#endif

    /*
     * The FQDM of this host or the empty string.
     */
#ifdef H5_HAVE_GETHOSTNAME
    if (HDgethostname(host_name, sizeof(host_name)) < 0)
        host_name[0] = '\0';
#else
    host_name[0] = '\0';
#endif

    /*
     * The file header: warning, copyright notice, build information.
     */
    HDfprintf(rawoutstream, "/* Generated automatically by H5make_libsettings -- do not edit */\n\n\n");
    HDfputs(FileHeader, rawoutstream); /*the copyright notice--see top of this file */

    HDfprintf(rawoutstream, " *\n * Created:\t\t%s %2d, %4d\n", month_name[tm->tm_mon], tm->tm_mday,
              1900 + tm->tm_year);
    if (pwd || real_name[0] || host_name[0]) {
        HDfprintf(rawoutstream, " *\t\t\t");
        if (real_name[0])
            HDfprintf(rawoutstream, "%s <", real_name);
#ifdef H5_HAVE_GETPWUID
        if (pwd)
            HDfputs(pwd->pw_name, rawoutstream);
#endif
        if (host_name[0])
            HDfprintf(rawoutstream, "@%s", host_name);
        if (real_name[0])
            HDfprintf(rawoutstream, ">");
        HDfputc('\n', rawoutstream);
    }

    HDfprintf(rawoutstream, " *\n * Purpose:\t\t");

    for (s = purpose; *s; s++) {
        HDfputc(*s, rawoutstream);
        if ('\n' == *s && s[1])
            HDfprintf(rawoutstream, " *\t\t\t");
    }

    HDfprintf(rawoutstream, " *\n * Modifications:\n *\n");
    HDfprintf(rawoutstream, " *\tDO NOT MAKE MODIFICATIONS TO THIS FILE!\n");
    HDfprintf(rawoutstream, " *\tIt was generated by code in `H5make_libsettings.c'.\n");

    HDfprintf(rawoutstream, " *\n *");
    for (i = 0; i < 73; i++)
        HDfputc('-', rawoutstream);
    HDfprintf(rawoutstream, "\n */\n\n");
}

/*-------------------------------------------------------------------------
 * Function:  print_footer
 *
 * Purpose:   Prints the file footer for the generated file.
 *
 * Return:    void
 *-------------------------------------------------------------------------
 */
static void
print_footer(void)
{
    /* nothing */
}

/*-------------------------------------------------------------------------
 * Function:  main
 *
 * Purpose:   Main entry point.
 *
 * Return:    Success:    EXIT_SUCCESS
 *-------------------------------------------------------------------------
 */
int
main(int argc, char *argv[])
{
    char *fname = NULL;
    FILE *f; /* temporary holding place for the stream pointer
              * so that rawoutstream is changed only when succeeded
              */

    if (argc > 1)
        fname = argv[1];

    /* First check if filename is string "NULL" */
    if (fname != NULL) {
        /* binary output */
        if ((f = HDfopen(fname, "w")) != NULL)
            rawoutstream = f;
    }
    if (!rawoutstream)
        rawoutstream = stdout;

    print_header();

    /* Generate embedded library information variable definition */
    make_libinfo();

    print_footer();

    if (rawoutstream && rawoutstream != stdout) {
        if (HDfclose(rawoutstream))
            HDfprintf(stderr, "closing rawoutstream");
        else
            rawoutstream = NULL;
    }

    HDexit(EXIT_SUCCESS);
}
