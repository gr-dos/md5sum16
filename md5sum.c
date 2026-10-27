/*
 * md5sum - compute and check MD5 message digests
 *
 * A GNU coreutils compatible md5sum for 16-bit MS-DOS (Open Watcom C
 * with libc16).
 */

#include <stdarg.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <errno.h>
#include <fcntl.h>
#include <io.h>
#include <dos.h>
#include <libc16.h>
#include "md5.h"

#ifndef VERSION
#define VERSION   "unknown"
#endif

#define BUFSIZE   16384
#define LINESIZE  512
#define PATHSIZE  144

#define MODE_DEFAULT 0      /* read binary, print "  " (matches Unix output) */
#define MODE_BINARY  1      /* read binary, print " *" */
#define MODE_TEXT    2      /* read text (CRLF -> LF, ^Z = EOF), print "  " */

static unsigned char iobuf[BUFSIZE];
static char linebuf[LINESIZE];

static int opt_mode = MODE_DEFAULT;
static int opt_tag = 0;
static int opt_quiet = 0;
static int opt_status = 0;
static int opt_warn = 0;
static int opt_strict = 0;
static int opt_ignore_missing = 0;

static const char *progname = "md5sum";

/* Writes the strings, up to a null pointer, to fp. */
static void put(FILE *fp, const char *s, ...)
{
    va_list ap;

    va_start(ap, s);
    for (; s != NULL; s = va_arg(ap, const char *))
        fputs(s, fp);
    va_end(ap);
}

/* "md5sum: A: B\n" on standard error */
static void error2(const char *a, const char *b)
{
    put(stderr, progname, ": ", a, ": ", b, "\n", NULL);
}

static void try_help(void)
{
    fputs("Try 'md5sum --help' for more information.\n", stderr);
}

static void usage(FILE *fp)
{
    fputs(
        "Usage: md5sum [OPTION]... [FILE]...\n"
        "Print or check MD5 (128-bit) checksums.\n"
        "With no FILE, or when FILE is -, read standard input.\n"
        "Wildcards (* and ?) in FILE are expanded.\n"
        "\n"
        "  -b, --binary   read in binary mode, mark lines with '*'\n"
        "  -c, --check    read checksums from the FILEs and check them\n"
        "      --tag      create a BSD-style checksum\n"
        "  -t, --text     read in text mode (CR/LF translated)\n"
        "  -z, --zero     (accepted for compatibility, ignored)\n"
        "\n"
        "The following options are useful only when verifying checksums:\n"
        "      --ignore-missing  don't fail or report status for missing files\n"
        "      --quiet    don't print OK for each successfully verified file\n"
        "      --status   don't output anything, status code shows success\n"
        "      --strict   exit non-zero for improperly formatted lines\n"
        "  -w, --warn     warn about improperly formatted checksum lines\n"
        "\n"
        "  -h, --help     display this help and exit\n"
        "      --version  output version information and exit\n", fp);
}

/* ---------------------------------------------------------------- hashing */

/* Hash an open handle.  In text mode CR LF becomes LF and a Ctrl-Z
   ends the file.  The handle is always binary: the Watcom runtime's
   text mode drops every CR and every Ctrl-Z and reads on past it.
   Returns 0 on success, -1 on read error. */
static int md5_handle(int fd, int text, unsigned char digest[16])
{
    MD5_CTX ctx;
    int n, i, j;
    int cr = 0;                 /* a CR ended the previous buffer */
    int eof = 0;

    md5_init(&ctx);
    while (!eof && (n = read(fd, iobuf, BUFSIZE)) > 0) {
        if (!text) {
            md5_update(&ctx, iobuf, (unsigned)n);
            continue;
        }
        if (cr && iobuf[0] != '\n')
            md5_update(&ctx, (unsigned char *)"\r", 1);
        cr = 0;
        for (i = j = 0; i < n; i++) {
            if (iobuf[i] == 0x1a) {
                eof = 1;
                break;
            }
            if (iobuf[i] == '\r') {
                if (i + 1 == n) {
                    cr = 1;
                    continue;
                }
                if (iobuf[i + 1] == '\n')
                    continue;
            }
            iobuf[j++] = iobuf[i];
        }
        md5_update(&ctx, iobuf, (unsigned)j);
    }
    if (cr)
        md5_update(&ctx, (unsigned char *)"\r", 1);
    md5_final(digest, &ctx);
    return n < 0 ? -1 : 0;
}

/* Hash a named file ("-" is stdin).  Returns 0 on success, -1 on error. */
static int md5_file(const char *name, int text, unsigned char digest[16])
{
    int fd, rc;

    if (strcmp(name, "-") == 0) {
        setmode(0, O_BINARY);
        return md5_handle(0, text, digest);
    }
    fd = open(name, O_RDONLY | O_BINARY);
    if (fd < 0)
        return -1;
    rc = md5_handle(fd, text, digest);
    close(fd);
    return rc;
}

static void print_hex(const unsigned char digest[16])
{
    static const char hex[] = "0123456789abcdef";
    int i;

    for (i = 0; i < 16; i++) {
        putchar(hex[digest[i] >> 4]);
        putchar(hex[digest[i] & 15]);
    }
}

static int sum_one(const char *name)
{
    unsigned char digest[16];

    if (md5_file(name, opt_mode == MODE_TEXT, digest) != 0) {
        error2(name, strerror(errno));
        return 1;
    }
    if (opt_tag) {
        put(stdout, "MD5 (", name, ") = ", NULL);
        print_hex(digest);
    } else {
        print_hex(digest);
        putchar(' ');
        putchar(opt_mode == MODE_BINARY ? '*' : ' ');
        fputs(name, stdout);
    }
    putchar('\n');
    return 0;
}

/* --------------------------------------------------------------- checking */

static int hexval(int c)
{
    if (c >= '0' && c <= '9') return c - '0';
    if (c >= 'a' && c <= 'f') return c - 'a' + 10;
    if (c >= 'A' && c <= 'F') return c - 'A' + 10;
    return -1;
}

static int parse_hex(const char *s, unsigned char digest[16])
{
    int i, hi, lo;

    for (i = 0; i < 16; i++) {
        hi = hexval(s[i * 2]);
        lo = hexval(s[i * 2 + 1]);
        if (hi < 0 || lo < 0)
            return -1;
        digest[i] = (unsigned char)((hi << 4) | lo);
    }
    return 0;
}

/* Undo GNU-style escaping of "\\" and "\n" in file names, in place. */
static void unescape(char *s)
{
    char *d = s;

    while (*s) {
        if (s[0] == '\\' && s[1] == '\\') {
            *d++ = '\\';
            s += 2;
        } else if (s[0] == '\\' && s[1] == 'n') {
            *d++ = '\n';
            s += 2;
        } else {
            *d++ = *s++;
        }
    }
    *d = '\0';
}

/*
 * Parse one checksum line, either
 *   [\]<32 hex><space><space|*><filename>
 * or the BSD form
 *   [\]MD5 (<filename>) = <32 hex>
 * Returns 0 and fills *name / digest / *text on success, -1 if malformed.
 */
static int parse_line(char *line, char **name, unsigned char digest[16],
                      int *text)
{
    int escaped = 0;
    char *p;
    size_t len;

    if (*line == '\\') {
        escaped = 1;
        line++;
    }

    if (strncmp(line, "MD5 (", 5) == 0) {
        len = strlen(line);
        if (len < 5 + 1 + 4 + 32)
            return -1;
        p = line + len - 32;
        if (strncmp(p - 4, ") = ", 4) != 0 || parse_hex(p, digest) != 0)
            return -1;
        p[-4] = '\0';
        *name = line + 5;
        *text = 0;
    } else {
        if (strlen(line) < 32 + 2 + 1)
            return -1;
        if (parse_hex(line, digest) != 0)
            return -1;
        if (line[32] != ' ' || (line[33] != ' ' && line[33] != '*'))
            return -1;
        *text = (line[33] == ' ' && opt_mode == MODE_TEXT);
        *name = line + 34;
    }
    if (**name == '\0')
        return -1;
    if (escaped)
        unescape(*name);
    return 0;
}

static int check_list(const char *listname)
{
    FILE *fp;
    unsigned char want[16], got[16];
    char *name;
    int text;
    unsigned long lineno = 0;
    unsigned long n_bad_format = 0, n_bad_read = 0, n_bad_sum = 0;
    unsigned long n_ok = 0;
    size_t len;
    int is_stdin = strcmp(listname, "-") == 0;
    int rc = 0;
    char num[12];

    if (is_stdin) {
        setmode(0, O_TEXT);
        fp = stdin;
    } else {
        fp = fopen(listname, "r");
        if (fp == NULL) {
            error2(listname, strerror(errno));
            return 1;
        }
    }

    while (fgets(linebuf, sizeof(linebuf), fp) != NULL) {
        lineno++;
        len = strlen(linebuf);
        if (len > 0 && linebuf[len - 1] != '\n' && !feof(fp)) {
            /* overlong line: discard the rest */
            int c;
            while ((c = getc(fp)) != EOF && c != '\n')
                ;
            linebuf[0] = '\0';
            len = 0;
        }
        while (len > 0 && (linebuf[len - 1] == '\n' || linebuf[len - 1] == '\r'))
            linebuf[--len] = '\0';
        if (linebuf[0] == '#' || len == 0)
            continue;

        if (parse_line(linebuf, &name, want, &text) != 0) {
            n_bad_format++;
            if (opt_warn)
                put(stderr, progname, ": ", listname, ": ",
                    libc16_ultoa(lineno, num, 10),
                    ": improperly formatted MD5 checksum line\n", NULL);
            continue;
        }

        if (md5_file(name, text, got) != 0) {
            if (opt_ignore_missing && access(name, 0) != 0)
                continue;
            n_bad_read++;
            if (!opt_status) {
                error2(name, strerror(errno));
                put(stdout, name, ": FAILED open or read\n", NULL);
            }
            continue;
        }

        if (memcmp(want, got, 16) == 0) {
            n_ok++;
            if (!opt_status && !opt_quiet)
                put(stdout, name, ": OK\n", NULL);
        } else {
            n_bad_sum++;
            if (!opt_status)
                put(stdout, name, ": FAILED\n", NULL);
        }
    }

    if (!is_stdin)
        fclose(fp);

    if (n_ok + n_bad_sum + n_bad_read == 0) {
        if (!opt_status) {
            if (opt_ignore_missing && n_bad_format == 0)
                error2(listname, "no file was verified");
            else
                error2(listname,
                       "no properly formatted MD5 checksum lines found");
        }
        return 1;
    }

    if (!opt_status) {
        if (n_bad_format)
            put(stderr, progname, ": WARNING: ",
                libc16_ultoa(n_bad_format, num, 10), " line",
                n_bad_format == 1 ? " is" : "s are", " improperly formatted\n",
                NULL);
        if (n_bad_read)
            put(stderr, progname, ": WARNING: ",
                libc16_ultoa(n_bad_read, num, 10), " listed file",
                n_bad_read == 1 ? "" : "s", " could not be read\n", NULL);
        if (n_bad_sum)
            put(stderr, progname, ": WARNING: ",
                libc16_ultoa(n_bad_sum, num, 10), " computed checksum",
                n_bad_sum == 1 ? "" : "s", " did NOT match\n", NULL);
    }

    if (n_bad_read || n_bad_sum || (opt_strict && n_bad_format))
        rc = 1;
    return rc;
}

/* -------------------------------------------------------------- wildcards */

static int has_wildcard(const char *s)
{
    return strchr(s, '*') != NULL || strchr(s, '?') != NULL;
}

/*
 * Expand a DOS wildcard and run fn on every matching file.  If nothing
 * matches, fn is called on the pattern itself so the error is reported.
 */
static int for_each_match(const char *pattern, int (*fn)(const char *))
{
    struct find_t ff;
    char path[PATHSIZE];
    size_t dirlen = 0;
    size_t i;
    unsigned matches = 0;
    int rc = 0;

    if (!has_wildcard(pattern) || strlen(pattern) >= PATHSIZE)
        return fn(pattern);

    for (i = 0; pattern[i]; i++)
        if (pattern[i] == '\\' || pattern[i] == '/' || pattern[i] == ':')
            dirlen = i + 1;

    if (_dos_findfirst(pattern, _A_NORMAL | _A_RDONLY | _A_HIDDEN | _A_SYSTEM
                       | _A_ARCH, &ff) == 0) {
        do {
            if (dirlen + strlen(ff.name) >= PATHSIZE)
                continue;
            memcpy(path, pattern, dirlen);
            strcpy(path + dirlen, ff.name);
            strlwr(path + dirlen);
            matches++;
            rc |= fn(path);
        } while (_dos_findnext(&ff) == 0);
    }

    if (matches == 0)
        return fn(pattern);
    return rc;
}

/* ------------------------------------------------------------------- main */

int main(int argc, char **argv)
{
    int check = 0;
    int i, nfiles = 0, endopts = 0;
    int rc = 0;
    int (*fn)(const char *);
    char opt[2] = { 0, 0 };

    for (i = 1; i < argc; i++) {
        const char *a = argv[i];

        if (endopts || a[0] != '-' || a[1] == '\0') {
            nfiles++;
            continue;
        }
        if (a[1] == '-') {
            if (a[2] == '\0')                       endopts = 1;
            else if (strcmp(a, "--binary") == 0)    opt_mode = MODE_BINARY;
            else if (strcmp(a, "--text") == 0)      opt_mode = MODE_TEXT;
            else if (strcmp(a, "--check") == 0)     check = 1;
            else if (strcmp(a, "--tag") == 0)       opt_tag = 1;
            else if (strcmp(a, "--zero") == 0)      ;
            else if (strcmp(a, "--quiet") == 0)     opt_quiet = 1;
            else if (strcmp(a, "--status") == 0)    opt_status = 1;
            else if (strcmp(a, "--warn") == 0)      opt_warn = 1;
            else if (strcmp(a, "--strict") == 0)    opt_strict = 1;
            else if (strcmp(a, "--ignore-missing") == 0) opt_ignore_missing = 1;
            else if (strcmp(a, "--help") == 0) {
                usage(stdout);
                return 0;
            } else if (strcmp(a, "--version") == 0) {
                fputs("md5sum (dos-md5sum16) " VERSION "\n", stdout);
                return 0;
            } else {
                put(stderr, progname, ": unrecognized option '", a, "'\n",
                    NULL);
                try_help();
                return 1;
            }
            argv[i] = NULL;
            continue;
        }
        for (a++; *a; a++) {
            switch (*a) {
            case 'b': opt_mode = MODE_BINARY; break;
            case 't': opt_mode = MODE_TEXT; break;
            case 'c': check = 1; break;
            case 'w': opt_warn = 1; break;
            case 'z': break;
            case 'h':
            case '?':
                usage(stdout);
                return 0;
            default:
                opt[0] = *a;
                put(stderr, progname, ": invalid option -- '", opt, "'\n",
                    NULL);
                try_help();
                return 1;
            }
        }
        argv[i] = NULL;
    }

    if (opt_tag && check) {
        put(stderr, progname, ": the --tag option is meaningless when "
            "verifying checksums\n", NULL);
        return 1;
    }
    if (opt_tag && opt_mode == MODE_TEXT) {
        put(stderr, progname, ": --tag does not support --text mode\n", NULL);
        return 1;
    }
    if (!check && (opt_quiet || opt_status || opt_warn || opt_strict
                   || opt_ignore_missing)) {
        put(stderr, progname, ": the --quiet, --status, --strict, --warn and "
            "--ignore-missing options are meaningful only when "
            "verifying checksums\n", NULL);
        return 1;
    }

    fn = check ? check_list : sum_one;

    if (nfiles == 0)
        return fn("-");

    for (i = 1; i < argc; i++)
        if (argv[i] != NULL)
            rc |= for_each_match(argv[i], fn);
    return rc;
}
