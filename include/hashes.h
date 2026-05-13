/*
 * hashes.h — NES port shared signatures.
 *
 * cc65 has no native uint64_t (see stdint.h: types stop at uint32_t),
 * so this header only declares algorithms whose state and arithmetic
 * fit in 32 bits. The dropped algos are:
 *
 *   crc64, fletcher64, xxh64, siphash24, sha512, sha3_256, sha3_512,
 *   murmur3_128
 *
 * Everything else (CRC-8/16/32, Adler-32, Fletcher-16/32, the tiny
 * non-crypto family, Murmur3-32, xxHash32, MD4, MD5, SHA-1, RIPEMD-160,
 * SHA-256, BLAKE2s, HMAC-SHA256, AES-CBC-MAC) ports as-is — the source
 * .c files use only uint8/16/32_t and stay under the cc65 limit.
 *
 * Why 64 bytes (not 1024 like NDS): NES has 2 KB internal RAM plus an
 * 8 KB WRAM bank at $6000-$8000. The benchmark buffer + cc65's 768-byte
 * parameter stack + heap/BSS plus stage buffers for crypto algos all
 * compete for that 8 KB. 64 bytes keeps the hot path comfortable while
 * still exercising ≥1 SHA-256 block.
 *
 * HBENCH_BANKED is a no-op here — the GB-only banked-call attribute
 * collapses to nothing on cc65 just like it does on devkitARM.
 */
#ifndef HASHES_H
#define HASHES_H

#include <stdint.h>

#ifndef HBENCH_BANKED
#  define HBENCH_BANKED
#endif

#define HASH_MAX_DIGEST 32u    /* largest enabled digest = SHA-256 / BLAKE2s */
#define BENCH_BUF_LEN   64u

/* ---- tiny non-cryptographic ------------------------------------------ */
void hash_crc8        (const uint8_t *data, uint16_t len, uint8_t out[1])  HBENCH_BANKED;
void hash_crc16       (const uint8_t *data, uint16_t len, uint8_t out[2])  HBENCH_BANKED;
void hash_crc32       (const uint8_t *data, uint16_t len, uint8_t out[4])  HBENCH_BANKED;
void hash_adler32     (const uint8_t *data, uint16_t len, uint8_t out[4])  HBENCH_BANKED;
void hash_fletcher16  (const uint8_t *data, uint16_t len, uint8_t out[2])  HBENCH_BANKED;
void hash_fletcher32  (const uint8_t *data, uint16_t len, uint8_t out[4])  HBENCH_BANKED;
void hash_djb2        (const uint8_t *data, uint16_t len, uint8_t out[4])  HBENCH_BANKED;
void hash_fnv1a32     (const uint8_t *data, uint16_t len, uint8_t out[4])  HBENCH_BANKED;
void hash_pearson     (const uint8_t *data, uint16_t len, uint8_t out[1])  HBENCH_BANKED;
void hash_knuth       (const uint8_t *data, uint16_t len, uint8_t out[4])  HBENCH_BANKED;
void hash_jenkins_oat (const uint8_t *data, uint16_t len, uint8_t out[4])  HBENCH_BANKED;
void hash_pjw_elf     (const uint8_t *data, uint16_t len, uint8_t out[4])  HBENCH_BANKED;
void hash_sdbm        (const uint8_t *data, uint16_t len, uint8_t out[4])  HBENCH_BANKED;

/* ---- non-cryptographic (more thorough) ------------------------------- */
void hash_murmur3     (const uint8_t *data, uint16_t len, uint8_t out[4])  HBENCH_BANKED;
void hash_xxh32       (const uint8_t *data, uint16_t len, uint8_t out[4])  HBENCH_BANKED;

/* ---- cryptographic --------------------------------------------------- */
void hash_md4         (const uint8_t *data, uint16_t len, uint8_t out[16]) HBENCH_BANKED;
void hash_md5         (const uint8_t *data, uint16_t len, uint8_t out[16]) HBENCH_BANKED;
void hash_sha1        (const uint8_t *data, uint16_t len, uint8_t out[20]) HBENCH_BANKED;

#endif
