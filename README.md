# md5sum16

A GNU coreutils compatible `md5sum` for 16-bit MS-DOS, built with Open
Watcom and [libc16](https://github.com/gr-dos/libc16).

The resulting `md5sum.exe` is a small-model, real-mode 8086 executable
(about 14 KB; 25 KB with Open Watcom's own runtime) that runs on any DOS
from an original PC up.

## Building

`md5sum.exe` links [libc16](https://github.com/gr-dos/libc16), a C
runtime for small-model DOS programs, instead of Open Watcom's own, and
is built with the Open Watcom toolchain that libc16 pins. By default
`make` runs `scripts/fetch-libc16`, which clones libc16 at the revision
pinned in that script into `build/libc16` and builds it there; libc16's
build fetches its Open Watcom snapshot and JWasm into
`build/libc16/build/tools`. This needs `git`, `curl`, `xz` and a host C
compiler. To move to a newer libc16, change the pin.

    make                         # fetches libc16 if needed, builds md5sum.exe
    make LIBC16_ROOT=../libc16   # or use a built local libc16 checkout
    make test                    # runs tests/run.sh under DOSBox-X
    make clean

The tests need `dosbox-x` and `md5sum` on the host (override with
`DOSBOX=` and `HOSTMD5=`); set `KEEP=1` to keep the work directory.

The version is the one line in `VERSION`, DOS style (0.01, 0.02, ...
1.00), not semantic versioning. When a PR that changes it is merged to
`main`, the release workflow tags the merge commit `v<VERSION>` and
publishes a GitHub release with `md5sum.exe` attached.

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
- `Makefile`: Open Watcom and libc16 build
- `scripts/fetch-libc16`: fetches and builds the pinned libc16
- `VERSION`: the version, compiled in and printed by `--version`
- `tests/run.sh`: the test suite, run under DOSBox-X by `make test`

## License

MIT. See `LICENSE`.
