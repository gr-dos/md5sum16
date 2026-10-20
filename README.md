# md5sum16

A GNU coreutils compatible `md5sum` for 16-bit MS-DOS, built with Open Watcom.

The resulting `md5sum.exe` is a small-model, real-mode 8086 executable
(about 25 KB) that runs on any DOS from an original PC up.

## Building

Requires Open Watcom (tested with the Linux-hosted toolchain).

    make                         # uses ~/opt/watcom by default
    make WATCOM=/path/to/watcom  # or point it elsewhere
    make clean

## Usage

    md5sum [OPTION]... [FILE]...

With no FILE, or when FILE is `-`, standard input is read. DOS does not
expand wildcards, so `md5sum` does it itself: `md5sum *.exe` works.

| Option | Meaning |
| --- | --- |
| `-b`, `--binary` | read in binary mode, mark lines with `*` |
| `-t`, `--text` | read in text mode (CR/LF translated to LF, Ctrl-Z ends the file) |
| `-c`, `--check` | read checksums from the FILEs and verify them |
| `--tag` | write BSD-style lines: `MD5 (file) = hash` |
| `-z`, `--zero` | accepted for compatibility, ignored |
| `--quiet` | (check) don't print `OK` for files that verify |
| `--status` | (check) print nothing; the exit code shows the result |
| `--strict` | (check) exit non-zero on improperly formatted lines |
| `-w`, `--warn` | (check) warn about improperly formatted lines |
| `--ignore-missing` | (check) skip files that don't exist |
| `-h`, `-?`, `--help` | show help |
| `--version` | show version |

The exit code is 0 on success and 1 if any file could not be read, any
checksum did not match, or (with `--strict`) any line was malformed.

### Examples

    md5sum *.zip > files.md5
    md5sum -c files.md5
    md5sum -c --quiet files.md5
    type readme.txt | md5sum

## Compatibility with Unix md5sum

- **Default mode** reads files byte for byte and prints two spaces between
  hash and name, exactly like Linux `md5sum`. Checksum lists move freely
  between DOS and Unix in either direction.
- **`-t`** hashes a DOS text file as its Unix copy would hash, by dropping
  the CRs. `-b` hashes the raw bytes, like the default, but marks the line
  with `*`.
- **Check files** may have CR/LF or LF line endings, use either the normal
  or the `--tag` format, and contain GNU-style escaped names (lines starting
  with `\`).
- File names found by wildcard expansion are printed in lower case; names
  given in full are printed as typed. Names are not escaped on output, since
  `\` is the DOS path separator.

## Limits

- Files are read in 16 KB chunks; file size is limited only by DOS.
- Lines in a check file may be up to 512 characters; longer lines are
  treated as malformed.

## Files

- `md5.c`, `md5.h`: MD5 (RFC 1321), written for 16-bit `int` / 32-bit `long`
- `md5sum.c`: the command-line program
- `Makefile`: Open Watcom build

## License

MIT. See `LICENSE`.
