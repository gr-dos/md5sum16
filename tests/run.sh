#!/bin/sh
#
# Test md5sum.exe under DOSBox-X.
#
# Fixtures and expected checksums are made on the host with the
# host's md5sum, one batch file runs every DOS-side step, and the results are
# checked on the host afterwards.  Each step N records md5sum's exit
# status in RN.TXT and its standard output in ON.TXT.  DOS cannot
# redirect standard error, so diagnostics are not checked.  rc.bat
# uses goto rather than "if errorlevel N echo ... >file" because the
# DOSBox-X shell opens the redirection even when the test is false.
#
# Usage: tests/run.sh [path/to/md5sum.exe]
# Environment: DOSBOX (default dosbox-x), HOSTMD5 (default md5sum),
#              KEEP=1 to keep the work directory.

set -u

top=$(cd "$(dirname "$0")/.." && pwd)
exe=${1:-$top/md5sum.exe}
DOSBOX=${DOSBOX:-dosbox-x}
HOSTMD5=${HOSTMD5:-md5sum}

command -v "$DOSBOX" >/dev/null || { echo "$DOSBOX not found" >&2; exit 2; }
command -v "$HOSTMD5" >/dev/null || { echo "$HOSTMD5 not found" >&2; exit 2; }
[ -f "$exe" ] || { echo "$exe not found" >&2; exit 2; }

work=$(mktemp -d "${TMPDIR:-/tmp}/dosmd5.XXXXXX")
pass=0
fail=0
LC_ALL=C
export LC_ALL

# ------------------------------------------------------------------
# fixtures

cd "$work" || exit 2
cp "$exe" md5sum.exe

printf 'hello\n' > a.txt
: > empty.dat
head -c 3000 /dev/urandom > b.bin
head -c 100000 /dev/urandom > big.bin
# text mode: CR LF becomes LF, a lone CR stays, Ctrl-Z ends the file;
# the reads are 16 KB, so split.txt and lone.txt put a CR at the end
# of the first buffer.  uN.txt is the Unix copy of tN.txt.
printf 'one\r\ntwo\r\n\032junk' > t1.txt
printf 'one\ntwo\n' > u1.txt
printf 'a\rb\r\r\n' > t2.txt
printf 'a\rb\r\n' > u2.txt
{ head -c 16383 /dev/zero | tr '\0' x; printf '\r\ny\r\n'; } > t3.txt
{ head -c 16383 /dev/zero | tr '\0' x; printf '\ny\n'; } > u3.txt
{ head -c 16383 /dev/zero | tr '\0' x; printf '\rz'; } > t4.txt
cp t4.txt u4.txt
printf 'end\r' > t5.txt
cp t5.txt u5.txt

mkdir wild
echo one > wild/one.c
echo two > wild/two.c
echo hdr > wild/x.h

FILES='a.txt empty.dat b.bin big.bin'
# shellcheck disable=SC2086
"$HOSTMD5" $FILES > good.md5
"$HOSTMD5" --tag $FILES > tag.md5
sed 's/$/\r/' good.md5 > crlf.md5
# a.txt with a wrong checksum
echo "$(head -c 32 good.md5 | tr '0-9a-f' 'f0-9a-e')  a.txt" > bad1.md5
cat good.md5 > malf.md5
echo 'this is not a checksum line' >> malf.md5
cat good.md5 > miss.md5
echo "d41d8cd98f00b204e9800998ecf8427e  nothere.txt" >> miss.md5

cat > rc.bat <<'EOF'
@echo off
if errorlevel 2 goto rc2
if errorlevel 1 goto rc1
echo rc0 >c:\r%1.txt
goto end
:rc1
echo rc1 >c:\r%1.txt
goto end
:rc2
echo rc2 >c:\r%1.txt
:end
EOF

cat > steps.bat <<'EOF'
@echo off
rem --- hash files
md5sum a.txt empty.dat b.bin big.bin >o1.txt
call rc 1
md5sum --tag a.txt empty.dat b.bin big.bin >o2.txt
call rc 2
md5sum -b a.txt >o3.txt
call rc 3
md5sum -t t1.txt t2.txt t3.txt t4.txt t5.txt >o4.txt
call rc 4
md5sum -t <t1.txt >o5.txt
call rc 5
md5sum <a.txt >o6.txt
call rc 6
md5sum - <a.txt >o7.txt
call rc 7
rem --- check
md5sum -c good.md5 >o8.txt
call rc 8
md5sum -c crlf.md5 >o9.txt
call rc 9
md5sum -c tag.md5 >o10.txt
call rc 10
md5sum -c bad1.md5 >o11.txt
call rc 11
md5sum -c --quiet bad1.md5 >o12.txt
call rc 12
md5sum -c --quiet good.md5 >o13.txt
call rc 13
md5sum -c --status bad1.md5 >o14.txt
call rc 14
md5sum -c --status good.md5 >o15.txt
call rc 15
md5sum -c malf.md5 >o16.txt
call rc 16
md5sum -c --strict malf.md5 >o17.txt
call rc 17
md5sum -c miss.md5 >o18.txt
call rc 18
md5sum -c --ignore-missing miss.md5 >o19.txt
call rc 19
rem --- round trip a list made on DOS
md5sum a.txt b.bin >dos.md5
md5sum -c dos.md5 >o20.txt
call rc 20
rem --- wildcards
md5sum wild\*.c >o21.txt
call rc 21
cd wild
..\md5sum *.zz >..\o22.txt
call ..\rc 22
cd ..
rem --- errors
md5sum nothere.txt >o23.txt
call rc 23
md5sum --bogus >o24.txt
call rc 24
md5sum -c --tag good.md5 >o25.txt
call rc 25
md5sum --quiet a.txt >o26.txt
call rc 26
rem --- help and version
md5sum --help >o27.txt
call rc 27
md5sum --version >o28.txt
call rc 28
EOF

SDL_VIDEODRIVER=dummy SDL_AUDIODRIVER=dummy timeout 300 \
    "$DOSBOX" -defaultconf -silent -nogui -nomenu \
    -set "cpu cycles=max" \
    -c "mount c \"$work\"" -c "c:" \
    -c "steps.bat" -c "exit" >dosbox.log 2>&1

# ------------------------------------------------------------------
# checks

ok()     { pass=$((pass + 1)); }
not_ok() { echo "FAIL: $*"; fail=$((fail + 1)); }

# check NAME COMMAND...
check() {
    name=$1
    shift
    if "$@"; then ok; else not_ok "$name"; fi
}

# rc STEP EXPECTED-STATUS
rc() {
    got=$(tr -d ' \r' < "R$1.TXT" 2>/dev/null | sed 's/^rc//')
    if [ "$got" = "$2" ]; then
        ok
    else
        not_ok "step $1: exit status ${got:-missing}, expected $2"
    fi
}

# expect STEP < expected-lines; compares ON.TXT with CRs removed
expect() {
    tr -d '\r' < "O$1.TXT" 2>/dev/null > got.txt
    cat > want.txt
    if cmp -s got.txt want.txt; then
        ok
    else
        not_ok "step $1: output differs"
        diff want.txt got.txt | sed 's/^/    /'
    fi
}

hash() { "$HOSTMD5" < "$1" | cut -c1-32; }

# hash files
rc 1 0
expect 1 < good.md5
rc 2 0
expect 2 < tag.md5
check "step 2: host md5sum checks --tag output" \
    sh -c "tr -d '\r' < O2.TXT | $HOSTMD5 -c --status"
rc 3 0
expect 3 <<EOF
$(hash a.txt) *a.txt
EOF
rc 4 0
expect 4 <<EOF
$(hash u1.txt)  t1.txt
$(hash u2.txt)  t2.txt
$(hash u3.txt)  t3.txt
$(hash u4.txt)  t4.txt
$(hash u5.txt)  t5.txt
EOF
rc 5 0
expect 5 <<EOF
$(hash u1.txt)  -
EOF
rc 6 0
expect 6 <<EOF
$(hash a.txt)  -
EOF
rc 7 0
expect 7 <<EOF
$(hash a.txt)  -
EOF

# check
for n in 8 9 10; do
    rc $n 0
    expect $n <<'EOF'
a.txt: OK
empty.dat: OK
b.bin: OK
big.bin: OK
EOF
done
rc 11 1
expect 11 <<'EOF'
a.txt: FAILED
EOF
rc 12 1
expect 12 <<'EOF'
a.txt: FAILED
EOF
rc 13 0
expect 13 < /dev/null
rc 14 1
expect 14 < /dev/null
rc 15 0
expect 15 < /dev/null
rc 16 0
expect 16 <<'EOF'
a.txt: OK
empty.dat: OK
b.bin: OK
big.bin: OK
EOF
rc 17 1
rc 18 1
check "step 18: missing file reported" \
    sh -c "tr -d '\r' < O18.TXT | grep -q '^nothere.txt: FAILED open or read$'"
rc 19 0
expect 19 <<'EOF'
a.txt: OK
empty.dat: OK
b.bin: OK
big.bin: OK
EOF

rc 20 0
expect 20 <<'EOF'
a.txt: OK
b.bin: OK
EOF
check "step 20: host md5sum checks DOS list" \
    sh -c "tr -d '\r' < DOS.MD5 | $HOSTMD5 -c --status"

# wildcards
rc 21 0
tr -d '\r' < O21.TXT | sort > got.txt
printf '%s  wild\\one.c\n%s  wild\\two.c\n' \
    "$(hash wild/one.c)" "$(hash wild/two.c)" > want.txt
check "step 21: wildcard expansion" cmp -s got.txt want.txt
rc 22 1
expect 22 < /dev/null

# errors
rc 23 1
expect 23 < /dev/null
rc 24 1
rc 25 1
rc 26 1

rc 27 0
check "step 27: help" grep -q '^Usage: md5sum' O27.TXT
rc 28 0
expect 28 <<EOF
md5sum (dos-md5sum16) $(cat "$top/VERSION")
EOF

# ------------------------------------------------------------------

echo "$pass passed, $fail failed"
if [ "$fail" -ne 0 ] || [ "${KEEP:-0}" = 1 ]; then
    echo "work directory: $work"
else
    rm -rf "$work"
fi
[ "$fail" -eq 0 ]
