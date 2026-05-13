# hash-bench-nes

Native **Nintendo Entertainment System / Famicom** hashing-algorithm
benchmark — **18 algorithms** on a 1.79 MHz 6502 (NTSC) with `clock()`-
driven timing, displayed in cc65's 32×30 software console. The
smallest-CPU sibling in the family; subset of the
[hash-bench-gba](https://github.com/dmang-dev/hash-bench-gba) /
[hash-bench-nds](https://github.com/dmang-dev/hash-bench-nds) /
[hash-bench-n64](https://github.com/dmang-dev/hash-bench-n64) lineup.

Why only 18 (vs. 32 on GBA / NDS): **cc65 has no `uint64_t`** — see
the note at the top of `I:\cc65\include\stdint.h`: "*not fully ISO
9899-1999 compliant because cc65 lacks 64 bit data types*". That
excludes any algorithm whose core state is a 64-bit integer:
`crc64`, `fletcher64`, `xxh64`, `siphash24`, `sha512`,
`sha3_{256,512}`, `murmur3_128`. The remaining 18 use `uint32_t`
(emulated through cc65's runtime — already painful enough on a
1.79 MHz 6502; see results below) and ports as-is from the rest
of the family.

| Tier | Algorithms |
|---|---|
| **Checksums (7)**       | CRC-8, CRC-16, CRC-32, Adler-32, Fletcher-16, Fletcher-32, Pearson-8 |
| **Non-crypto (8)**      | DJB2, FNV-1a, Knuth, Jenkins-OAT, PJW/ELF, SDBM, Murmur3-32, xxHash32 |
| **Cryptographic (3)**   | MD4, MD5, SHA-1 |

[![ROM](https://img.shields.io/badge/ROM-prebuilt%20%26%20committed-success)](hash-bench-nes.nes)
[![Built with cc65](https://img.shields.io/badge/built%20with-cc65%20%2B%20nes.lib-orange)](https://cc65.github.io)
[![Mapper](https://img.shields.io/badge/mapper-NROM%20%280%29-lightgrey)](https://www.nesdev.org/wiki/NROM)

---

## What it does

On boot, cc65's `nes.lib` text console comes up at 32×30 with the
default font CHR. The harness then:

1. Fills a 64-byte buffer with the canonical pattern
   `buf[i] = (i * 31 + 7) & 0xFF`.
2. Walks each algorithm in sequence, displaying a live
   `running N/18 LABEL` row at line 1.
3. For each algo, calls `clock()` (cc65 NES nominal `CLOCKS_PER_SEC =
   50`) and loops the hash until 25 ticks have elapsed (~500 ms wall).
4. Stores `(iterations, ticks, first-digest-byte)` into a 6-byte
   `algo_result` per algorithm.
5. After all 18 finish, renders the full table in one pass:

```
hash-bench-nes v2
64B  budget=500ms
ALGO    ITER MS/IT H
--------------------
. CRC8     16    32 BB
. CRC16     8    65 79
. CRC32     4   145 88
. ADL32    22    22 FF
. FLT16     8    62 43
. FLT32     4   130 C8
. PRSN8    36    13 5A
* KNUTH     5   120 0E
* OAT      12    41 74
* PJW       7    74 09
* SDBM     11    47 2F
* DJB2     14    35 BE
* FNV1A     5   108 58
* MMUR3     5   108 4B
* XXH32     6    83 F2
# MD4       3   240 11
# MD5       2   450 B6
# SHA1      1   640 39
any btn: rerun
```

Tier markers in the leading column: `.` = checksum, `*` = non-crypto,
`#` = crypto. Press any joypad button to rerun.

---

## Try it

Pre-built ROM in the repo root: [`hash-bench-nes.nes`](hash-bench-nes.nes)
(40 976 bytes — 16 B iNES header + 32 KB PRG ROM + 8 KB CHR ROM).

**Recommended emulator: [Mesen2](https://github.com/SourMesen/Mesen2)**
or its predecessor [Mesen](https://github.com/SourMesen/Mesen).
cc65's NES configuration (`nes.cfg`) places `BSS` at `$6000-$7FFF`
— the "8 KB SRAM Bank" region. Our iNES header sets the battery-PRG-RAM
flag (`flags6 bit 1`) to signal this, and Mesen honors that flag and
maps an 8 KB WRAM window. **FCEUX is stricter about NROM not having
WRAM** and may fail to map the region; the symptom is a partial run
that freezes after a few algorithms (writes to `results[]` and
`buf[]` go to open bus, reads return garbage, the loop counter
corrupts).

Other accurate options: **Nestopia UE**, **puNES**, **higan/bsnes**
(NES core).

---

## Build from source

Requires **cc65** in PATH. On Windows the bundled
`build.bat` expects `I:\cc65\bin\cl65.exe` — adjust to your install
path or set `PATH` accordingly.

```
.\build.bat            # build hash-bench-nes.nes
```

`build.bat` is a thin wrapper around `cl65 -t nes -O -I include`. It
enumerates `source\*.c` via a `for` loop because `cl65` doesn't
expand globs itself. Build artifacts go to `build\`; the final ROM
lands in the project root.

The default `cc65` standard (which we use) is C89 + cc65 extensions.
**Don't pass `--standard c99`** — strict C99 mode rejects the
`extern void joy_static_stddrv[]` array declaration that
`<joystick.h>` and `<nes.h>` rely on.

---

## Smoketests

Two minimal ROMs in [`tests/`](tests/) for triaging emulator-compat
issues without dragging the full algorithm set along:

- [`smoketest.c`](tests/smoketest.c) — `clrscr` + `cputs` + a
  `clock()`-driven counter that ticks once per second. Proves the
  cc65 NES toolchain produces working text-output ROMs on a given
  emulator.
- [`smoketest2.c`](tests/smoketest2.c) — same init flow as `main.c`
  but with a no-op "bench" body. Proves the harness loop structure
  works even if a real algorithm doesn't.

Build either via `build-smoketest.bat` / `build-smoketest2.bat` —
they live in the project root and link only their own source.

---

## Algorithms

All 14 algorithm `.c` files are byte-identical to
[`hash-bench-gba/source/`](https://github.com/dmang-dev/hash-bench-gba/tree/main/source);
the 18-algo subset matches what
[`hash-bench-gb`](https://github.com/dmang-dev/hash-bench-gb) ships
(both targets get squeezed by their respective uint32-runtime
overhead). The only NES-specific edit is in
[`source/fletcher.c`](source/fletcher.c) — `hash_fletcher64()` is
gated behind `#if !defined(__PORT_sm83) && !defined(__NES__)` because
its 64-bit modulo would require a `__moddi3` runtime that doesn't
exist on cc65.

### Reference digests

Workload buffer: 64 bytes of `(i * 31 + 7) & 0xFF` for `i ∈ [0, 64)`.
The displayed `H` column shows the *first byte* of the digest only —
just enough to confirm the algorithm produced *some* deterministic
output without spending screen real-estate on full hex dumps.

| Algo | First byte | Full digest (64 B input) |
|---|---|---|
| CRC-8       | `BB` | `BB` |
| CRC-16      | `79` | `79CB` |
| CRC-32      | `88` | `881BAA8C` |
| Adler-32    | `FF` | `FFFFEC4F` |
| Fletcher-16 | `43` | `434B` |
| Fletcher-32 | `C8` | `C8E48F23` |
| Pearson-8   | `5A` | `5A` |
| Knuth       | `0E` | `0E000000` |
| Jenkins-OAT | `74` | `746A2A91` |
| PJW/ELF     | `09` | `09F1AB22` |
| SDBM        | `2F` | `2F84BB80` |
| DJB2        | `BE` | `BEBC4E84` |
| FNV-1a      | `58` | `58E0C5C7` |
| Murmur3-32  | `4B` | `4B...` |
| xxHash32    | `F2` | `F2...` |
| MD4         | `11` | `11...` |
| MD5         | `B6` | `B6...` |
| SHA-1       | `39` | `39...` |

(Full reference digests for the 1024-byte workload — used by
[hash-bench-nds](https://github.com/dmang-dev/hash-bench-nds#reference-digests)
— don't match here because we benchmark a 64-byte buffer.)

---

## Why the numbers look the way they do

Looking at the sample output above:

- **Pearson-8 wins** at 36 iters / 13 ms — single 256-byte table
  lookup per byte, no 32-bit math anywhere.
- **Adler-32 surprisingly competitive** (22 iters) — its inner loop
  is just two adds in 8-bit registers; the modulo runs only every 256
  iterations.
- **Knuth multiplicative slow** (5 iters) despite being only one
  operation per byte — that operation is a `uint32 *= 0x9E3779B1`,
  which on a 6502 means a call into cc65's `__mulul` runtime (32×32-
  bit multiply via shift-and-add, ~2000 cycles). DJB2 and SDBM avoid
  this by using shifts/adds only.
- **Crypto tier crawls** — MD4 at 240 ms/iter, SHA-1 at 640 ms/iter.
  Each round of MD/SHA-family compresses 4× `uint32_t` operations,
  every one going through the cc65 long runtime. SHA-1's 80 rounds
  vs MD4's 48 explains the ~3× gap.

The same algorithms run 100-1000× faster on GBA / NDS (32-bit ARM
with native long multiply) and another 50× faster on N64 (64-bit
MIPS3). See the cross-platform comparison in
[hash-bench-nds#cross-validation](https://github.com/dmang-dev/hash-bench-nds#cross-validation).

---

## Timing methodology

cc65 declares `CLOCKS_PER_SEC = 50` on NES (per
`I:\cc65\include\time.h`). Whether `clock()` actually ticks at 50 Hz
or 60 Hz (NTSC vblank rate) is implementation-defined inside `nes.lib`;
the on-screen `MS/IT` column trusts the header value and computes
`ticks * 20 / iters`. The numbers are useful for *relative*
comparison between algorithms on the same console; for absolute
ms-per-iter you may need to multiply by 50/60 (NTSC) or 50/60 (PAL —
PAL NES vblank is 50 Hz so the published value happens to be exact
there).

Per-algorithm budget: 25 ticks (`BUDGET_TICKS`), nominally 500 ms.
The harness keeps iterating until `clock() - t0 >= 25`, then records
the actual elapsed ticks. Slow crypto algorithms overshoot — SHA-1's
single iteration takes 640 ms (32 ticks) — and that overshoot is
captured honestly in the `MS/IT` column.

---

## Layout

```
source/                 Pure-C sources
  main.c                  Init, switch-dispatch, render
  crc8.c crc16.c crc32.c              checksums          (shared)
  adler32.c fletcher.c pearson.c      checksums          (shared)
  djb2.c fnv1a.c tiny_hashes.c        non-crypto         (shared)
  murmur3.c xxhash32.c                non-crypto         (shared)
  md4.c md5.c sha1.c                  crypto             (shared)
include/
  hashes.h                Function declarations (NES port subset)
tests/
  smoketest.c             Toolchain sanity ROM (no algos)
  smoketest2.c            Init-flow sanity ROM (no algos)
build/                  cl65 object files + map (gitignored)
Makefile                cl65 driver — `make` works on Linux/macOS
build.bat               Windows wrapper, expects I:\cc65\bin\
build-smoketest{,2}.bat Build the diagnostic ROMs
```

`main.c` uses **direct switch-dispatch** rather than a function-
pointer table — cc65's fastcall convention can mishandle pointer
casts across functions whose final argument is a typed array of
varying size (`uint8_t out[1]` vs `uint8_t out[16]` etc.). The
switch costs one extra `cmp/bne` per algorithm but yields rock-solid
calling.

---

## Acknowledgments

- [cc65](https://cc65.github.io/) — 6502 C compiler + NES support
- [Mesen2](https://github.com/SourMesen/Mesen2) — accurate NES
  emulator that maps NROM-with-WRAM correctly
- [hash-bench-gb](https://github.com/dmang-dev/hash-bench-gb) /
  [hash-bench-gba](https://github.com/dmang-dev/hash-bench-gba) /
  [hash-bench-nds](https://github.com/dmang-dev/hash-bench-nds) /
  [hash-bench-n64](https://github.com/dmang-dev/hash-bench-n64) —
  sibling ports supplying the algorithm sources
