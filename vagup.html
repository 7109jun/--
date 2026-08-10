/*
 * vag_codec.c
 *
 * VAG용 무손실 압축/해제 코어 모듈
 *
 * 지원 방법:
 * - VAG_COMP_NONE          : 무압축
 * - VAG_COMP_HUFF          : Huffman only
 * - VAG_COMP_LZ_HUFF       : LZSS(중복 패턴 대체) + Huffman
 * - VAG_COMP_GZIP          : Gzip (zlib 필요, VAG_USE_ZLIB)
 * - VAG_COMP_LZ_HUFF_GZIP  : LZSS + Huffman 후 다시 Gzip (zlib 필요)
 *
 * 빌드 예:
 *   gcc -std=c99 -c vag_codec.c
 *   gcc -std=c99 -DVAG_USE_ZLIB vag_codec.c -lz
 *
 * C++로 빌드:
 *   g++ -std=c++17 -c vag_codec.cpp
 *   g++ -std=c++17 -DVAG_USE_ZLIB vag_codec.cpp -lz
 */

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <stdint.h>
#include <limits.h>

#ifdef VAG_USE_ZLIB
#include <zlib.h>
#endif

#ifdef __cplusplus
extern "C" {
#endif

#define VAG_CODEC_MAGIC "VCP1"
#define VAG_CODEC_MAGIC_LEN 4
#define VAG_CODEC_HEADER_SIZE 13u

#define VAG_HUFF_MAX_LEN 64

#define VAG_LZ_WINDOW 32768u
#define VAG_LZ_MIN_MATCH 6u
#define VAG_LZ_MAX_MATCH 1024u
#define VAG_LZ_HASH_SIZE 65536u

typedef enum {
    VAGC_OK = 0,
    VAGC_ERR_NULL,
    VAGC_ERR_NO_MEMORY,
    VAGC_ERR_CORRUPT,
    VAGC_ERR_IO,
    VAGC_ERR_UNSUPPORTED,
    VAGC_ERR_TOO_LARGE,
    VAGC_ERR_BAD_MAGIC
} vagc_error;

typedef enum {
    VAG_COMP_NONE = 0,
    VAG_COMP_HUFF = 1,
    VAG_COMP_LZ_HUFF = 2,
    VAG_COMP_GZIP = 3,
    VAG_COMP_LZ_HUFF_GZIP = 4
} vag_comp_method;

const char* vagc_strerror(vagc_error err);

/*
 * 압축 API
 *
 * 입력 in/in_size를 method 방식으로 압축해서 malloc된 blob을 만든다.
 * blob 헤더에 method/original_size가 들어간다.
 *
 * 압축 결과가 무압축보다 커지면 자동으로 NONE으로 fallback 한다.
 */
vagc_error vag_compress_data(vag_comp_method method,
                             const uint8_t* in,
                             size_t in_size,
                             uint8_t** out,
                             size_t* out_size);

/*
 * 해제 API
 *
 * vag_compress_data가 만든 blob을 풀어 원본 데이터를 만든다.
 */
vagc_error vag_decompress_data(const uint8_t* blob,
                               size_t blob_size,
                               uint8_t** out,
                               size_t* out_size);

/* ------------------------------------------------------------------ */
/* Internal helpers                                                    */
/* ------------------------------------------------------------------ */

typedef struct {
    uint8_t* data;
    size_t len;
    size_t cap;
} vagc_buf;

static int vagc_buf_init(vagc_buf* b, size_t cap)
{
    b->data = NULL;
    b->len = 0;
    b->cap = 0;

    if (cap == 0) {
        return 1;
    }

    b->data = (uint8_t*)malloc(cap);
    if (!b->data) {
        return 0;
    }

    b->cap = cap;
    return 1;
}

static void vagc_buf_free(vagc_buf* b)
{
    if (!b) {
        return;
    }

    free(b->data);
    b->data = NULL;
    b->len = 0;
    b->cap = 0;
}

static int vagc_buf_reserve(vagc_buf* b, size_t extra)
{
    if (extra > SIZE_MAX - b->len) {
        return 0;
    }

    size_t need = b->len + extra;

    if (need <= b->cap) {
        return 1;
    }

    size_t cap = (b->cap != 0) ? b->cap : 64;

    while (cap < need) {
        if (cap > SIZE_MAX / 2) {
            cap = need;
            break;
        }
        cap *= 2;
    }

    uint8_t* tmp = (uint8_t*)realloc(b->data, cap);
    if (!tmp) {
        return 0;
    }

    b->data = tmp;
    b->cap = cap;

    return 1;
}

static int vagc_buf_append(vagc_buf* b, const void* data, size_t n)
{
    if (n == 0) {
        return 1;
    }

    if (!vagc_buf_reserve(b, n)) {
        return 0;
    }

    memcpy(b->data + b->len, data, n);
    b->len += n;

    return 1;
}

static int vagc_buf_push_u8(vagc_buf* b, uint8_t v)
{
    if (!vagc_buf_reserve(b, 1)) {
        return 0;
    }

    b->data[b->len++] = v;
    return 1;
}

static void vagc_put_u16(uint8_t* p, uint16_t v)
{
    p[0] = (uint8_t)(v & 0xFFu);
    p[1] = (uint8_t)((v >> 8) & 0xFFu);
}

static void vagc_put_u64(uint8_t* p, uint64_t v)
{
    int i;
    for (i = 0; i < 8; ++i) {
        p[i] = (uint8_t)((v >> (8 * i)) & 0xFFu);
    }
}

static uint16_t vagc_get_u16(const uint8_t* p)
{
    return (uint16_t)((uint16_t)p[0] | ((uint16_t)p[1] << 8));
}

static uint64_t vagc_get_u64(const uint8_t* p)
{
    uint64_t v = 0;
    int i;
    for (i = 0; i < 8; ++i) {
        v |= (uint64_t)p[i] << (8 * i);
    }
    return v;
}

static int vagc_buf_push_u16(vagc_buf* b, uint16_t v)
{
    uint8_t tmp[2];
    vagc_put_u16(tmp, v);
    return vagc_buf_append(b, tmp, 2);
}

/* ------------------------------------------------------------------ */
/* Bit writer/reader                                                   */
/* ------------------------------------------------------------------ */

typedef struct {
    vagc_buf* out;
    uint8_t acc;
    int bits;
} vagc_bitwriter;

static int vagc_bw_write_bit(vagc_bitwriter* w, int bit)
{
    w->acc = (uint8_t)((w->acc << 1) | (bit ? 1 : 0));
    w->bits++;

    if (w->bits == 8) {
        if (!vagc_buf_push_u8(w->out, w->acc)) {
            return 0;
        }
        w->acc = 0;
        w->bits = 0;
    }

    return 1;
}

static int vagc_bw_write_bits(vagc_bitwriter* w, uint64_t code, int len)
{
    int i;

    if (len < 0 || len > 64) {
        return 0;
    }

    for (i = len - 1; i >= 0; --i) {
        int bit = (int)((code >> i) & 1u);

        if (!vagc_bw_write_bit(w, bit)) {
            return 0;
        }
    }

    return 1;
}

static int vagc_bw_flush(vagc_bitwriter* w)
{
    if (w->bits == 0) {
        return 1;
    }

    uint8_t byte = (uint8_t)(w->acc << (8 - w->bits));

    if (!vagc_buf_push_u8(w->out, byte)) {
        return 0;
    }

    w->acc = 0;
    w->bits = 0;

    return 1;
}

typedef struct {
    const uint8_t* data;
    size_t size;
    size_t pos;
    int bit_pos; /* 0..7, MSB first */
} vagc_bitreader;

static int vagc_br_read_bit(vagc_bitreader* r, int* out_bit)
{
    if (r->pos >= r->size) {
        return 0;
    }

    uint8_t byte = r->data[r->pos];

    *out_bit = (int)((byte >> (7 - r->bit_pos)) & 1u);

    r->bit_pos++;

    if (r->bit_pos == 8) {
        r->bit_pos = 0;
        r->pos++;
    }

    return 1;
}

/* ------------------------------------------------------------------ */
/* Huffman                                                             */
/* ------------------------------------------------------------------ */

typedef struct {
    uint64_t weight;
    int left;
    int right;
    int parent;
    int symbol;
} vagc_huff_node;

static vagc_error vagc_build_huffman_lengths(const uint64_t freq[256],
                                             uint8_t lengths[256],
                                             int* max_len_out)
{
    vagc_huff_node nodes[512];
    int node_count = 0;
    int i;

    memset(lengths, 0, 256);
    memset(nodes, 0, sizeof(nodes));

    for (i = 0; i < 256; ++i) {
        if (freq[i] > 0) {
            nodes[node_count].weight = freq[i];
            nodes[node_count].left = -1;
            nodes[node_count].right = -1;
            nodes[node_count].parent = -1;
            nodes[node_count].symbol = i;
            node_count++;
        }
    }

    if (node_count == 0) {
        *max_len_out = 0;
        return VAGC_OK;
    }

    if (node_count == 1) {
        lengths[nodes[0].symbol] = 1;
        *max_len_out = 1;
        return VAGC_OK;
    }

    int root_count = node_count;

    while (root_count > 1) {
        int first = -1;
        int second = -1;

        for (i = 0; i < node_count; ++i) {
            if (nodes[i].parent != -1) {
                continue;
            }

            if (first == -1 || nodes[i].weight < nodes[first].weight) {
                second = first;
                first = i;
            } else if (second == -1 || nodes[i].weight < nodes[second].weight) {
                second = i;
            }
        }

        if (first == -1 || second == -1) {
            return VAGC_ERR_CORRUPT;
        }

        if (node_count >= 512) {
            return VAGC_ERR_CORRUPT;
        }

        nodes[node_count].weight = nodes[first].weight + nodes[second].weight;
        nodes[node_count].left = first;
        nodes[node_count].right = second;
        nodes[node_count].parent = -1;
        nodes[node_count].symbol = -1;

        nodes[first].parent = node_count;
        nodes[second].parent = node_count;

        node_count++;
        root_count--;
    }

    int max_len = 0;

    for (i = 0; i < node_count; ++i) {
        if (nodes[i].symbol < 0) {
            continue;
        }

        int depth = 0;
        int p = nodes[i].parent;

        while (p != -1) {
            depth++;

            if (depth > VAG_HUFF_MAX_LEN) {
                return VAGC_ERR_TOO_LARGE;
            }

            p = nodes[p].parent;
        }

        lengths[nodes[i].symbol] = (uint8_t)depth;

        if (depth > max_len) {
            max_len = depth;
        }
    }

    *max_len_out = max_len;

    return VAGC_OK;
}

static vagc_error vagc_make_canonical_codes(const uint8_t lengths[256],
                                            uint64_t codes[256],
                                            int* max_len_out)
{
    int counts[VAG_HUFF_MAX_LEN + 1];
    uint64_t next_code[VAG_HUFF_MAX_LEN + 1];

    memset(counts, 0, sizeof(counts));
    memset(next_code, 0, sizeof(next_code));
    memset(codes, 0, 256 * sizeof(uint64_t));

    int i;

    for (i = 0; i < 256; ++i) {
        if (lengths[i] > VAG_HUFF_MAX_LEN) {
            return VAGC_ERR_TOO_LARGE;
        }

        if (lengths[i] > 0) {
            counts[lengths[i]]++;
        }
    }

    uint64_t code = 0;
    int max_len = 0;

    int bits;

    for (bits = 1; bits <= VAG_HUFF_MAX_LEN; ++bits) {
        code = (code + (uint64_t)counts[bits - 1]) << 1;
        next_code[bits] = code;

        if (counts[bits] > 0) {
            max_len = bits;
        }
    }

    for (i = 0; i < 256; ++i) {
        if (lengths[i] > 0) {
            codes[i] = next_code[lengths[i]]++;
        }
    }

    *max_len_out = max_len;

    return VAGC_OK;
}

/*
 * Huffman payload:
 *   uint8 code_lengths[256]
 *   bitstream
 */
static vagc_error vagc_huffman_encode_payload(const uint8_t* symbols,
                                              size_t symbol_count,
                                              uint8_t** out,
                                              size_t* out_size)
{
    uint64_t freq[256];
    uint8_t lengths[256];
    uint64_t codes[256];
    int max_len = 0;
    vagc_error err = VAGC_OK;
    vagc_buf b;
    size_t i;

    *out = NULL;
    *out_size = 0;

    memset(freq, 0, sizeof(freq));

    if (symbol_count > 0 && !symbols) {
        return VAGC_ERR_NULL;
    }

    for (i = 0; i < symbol_count; ++i) {
        freq[symbols[i]]++;
    }

    err = vagc_build_huffman_lengths(freq, lengths, &max_len);
    if (err != VAGC_OK) {
        return err;
    }

    if (!vagc_buf_init(&b, 256)) {
        return VAGC_ERR_NO_MEMORY;
    }

    if (!vagc_buf_append(&b, lengths, sizeof(lengths))) {
        err = VAGC_ERR_NO_MEMORY;
        goto fail;
    }

    if (symbol_count > 0) {
        if (max_len == 0) {
            err = VAGC_ERR_CORRUPT;
            goto fail;
        }

        err = vagc_make_canonical_codes(lengths, codes, &max_len);
        if (err != VAGC_OK) {
            goto fail;
        }

        vagc_bitwriter bw;
        bw.out = &b;
        bw.acc = 0;
        bw.bits = 0;

        for (i = 0; i < symbol_count; ++i) {
            uint8_t sym = symbols[i];

            if (lengths[sym] == 0) {
                err = VAGC_ERR_CORRUPT;
                goto fail;
            }

            if (!vagc_bw_write_bits(&bw, codes[sym], lengths[sym])) {
                err = VAGC_ERR_NO_MEMORY;
                goto fail;
            }
        }

        if (!vagc_bw_flush(&bw)) {
            err = VAGC_ERR_NO_MEMORY;
            goto fail;
        }
    }

    *out = b.data;
    *out_size = b.len;

    return VAGC_OK;

fail:
    vagc_buf_free(&b);
    return err;
}

static vagc_error vagc_huffman_decode_payload(const uint8_t* payload,
                                              size_t payload_size,
                                              size_t symbol_count,
                                              uint8_t** out,
                                              size_t* out_size)
{
    *out = NULL;
    *out_size = 0;

    if (payload_size < 256) {
        return VAGC_ERR_CORRUPT;
    }

    const uint8_t* lengths = payload;

    if (symbol_count == 0) {
        return VAGC_OK;
    }

    int counts[VAG_HUFF_MAX_LEN + 1];
    uint64_t first_code[VAG_HUFF_MAX_LEN + 1];
    int first_index[VAG_HUFF_MAX_LEN + 1];
    int order[256];

    memset(counts, 0, sizeof(counts));

    int max_len = 0;
    int i;

    for (i = 0; i < 256; ++i) {
        uint8_t len = lengths[i];

        if (len > VAG_HUFF_MAX_LEN) {
            return VAGC_ERR_CORRUPT;
        }

        if (len > 0) {
            counts[len]++;

            if (len > max_len) {
                max_len = len;
            }
        }
    }

    if (max_len == 0) {
        return VAGC_ERR_CORRUPT;
    }

    uint64_t code = 0;
    int idx = 0;

    int bits;

    for (bits = 1; bits <= max_len; ++bits) {
        first_code[bits] = code;
        first_index[bits] = idx;

        int sym;

        for (sym = 0; sym < 256; ++sym) {
            if (lengths[sym] == bits) {
                order[idx++] = sym;
            }
        }

        code = (code + (uint64_t)counts[bits]) << 1;
    }

    vagc_buf b;

    if (!vagc_buf_init(&b, symbol_count)) {
        return VAGC_ERR_NO_MEMORY;
    }

    vagc_bitreader br;
    br.data = payload + 256;
    br.size = payload_size - 256;
    br.pos = 0;
    br.bit_pos = 0;

    vagc_error err = VAGC_OK;

    for (i = 0; i < (int)symbol_count; ++i) {
        uint64_t cur_code = 0;
        int len = 0;
        int found = 0;

        while (len < max_len) {
            int bit = 0;

            if (!vagc_br_read_bit(&br, &bit)) {
                err = VAGC_ERR_CORRUPT;
                goto fail;
            }

            cur_code = (cur_code << 1) | (uint64_t)bit;
            len++;

            if (counts[len] > 0) {
                uint64_t start = first_code[len];

                if (cur_code >= start && cur_code < start + (uint64_t)counts[len]) {
                    int rel = (int)(cur_code - start);
                    int sym = order[first_index[len] + rel];

                    if (!vagc_buf_push_u8(&b, (uint8_t)sym)) {
                        err = VAGC_ERR_NO_MEMORY;
                        goto fail;
                    }

                    found = 1;
                    break;
                }
            }
        }

        if (!found) {
            err = VAGC_ERR_CORRUPT;
            goto fail;
        }
    }

    *out = b.data;
    *out_size = b.len;

    return VAGC_OK;

fail:
    vagc_buf_free(&b);
    return err;
}

/* ------------------------------------------------------------------ */
/* LZSS (중복 패턴 대체)                                               */
/* ------------------------------------------------------------------ */

static uint32_t vagc_lz_hash3(const uint8_t* p)
{
    uint32_t h;

    h = (uint32_t)p[0];
    h ^= ((uint32_t)p[1] << 8);
    h ^= ((uint32_t)p[2] << 16);

    return h & (VAG_LZ_HASH_SIZE - 1u);
}

/*
 * LZSS 출력 스트림:
 *
 * literal:
 *   0x00, byte
 *
 * match:
 *   0x01, uint16 offset, uint16 length
 */
static vagc_error vagc_lzss_encode(const uint8_t* in,
                                   size_t in_size,
                                   vagc_buf* out)
{
    uint64_t* last = NULL;
    size_t pos = 0;
    uint32_t i;

    if (in_size > 0 && !in) {
        return VAGC_ERR_NULL;
    }

    last = (uint64_t*)malloc(sizeof(uint64_t) * VAG_LZ_HASH_SIZE);
    if (!last) {
        return VAGC_ERR_NO_MEMORY;
    }

    for (i = 0; i < VAG_LZ_HASH_SIZE; ++i) {
        last[i] = UINT64_MAX;
    }

    while (pos < in_size) {
        size_t best_len = 0;
        uint64_t best_off = 0;

        if (pos + 3 <= in_size) {
            uint32_t h = vagc_lz_hash3(in + pos);
            uint64_t cand = last[h];

            if (cand != UINT64_MAX && cand < (uint64_t)pos) {
                uint64_t distance = (uint64_t)pos - cand;

                if (distance <= VAG_LZ_WINDOW) {
                    size_t c = (size_t)cand;

                    size_t max_match = in_size - pos;

                    if (max_match > VAG_LZ_MAX_MATCH) {
                        max_match = VAG_LZ_MAX_MATCH;
                    }

                    size_t l = 0;

                    while (l < max_match && in[c + l] == in[pos + l]) {
                        ++l;
                    }

                    if (l >= VAG_LZ_MIN_MATCH) {
                        best_len = l;
                        best_off = distance;
                    }
                }
            }

            last[h] = (uint64_t)pos;
        }

        if (best_len >= VAG_LZ_MIN_MATCH) {
            if (!vagc_buf_push_u8(out, 1)) {
                free(last);
                return VAGC_ERR_NO_MEMORY;
            }

            if (!vagc_buf_push_u16(out, (uint16_t)best_off)) {
                free(last);
                return VAGC_ERR_NO_MEMORY;
            }

            if (!vagc_buf_push_u16(out, (uint16_t)best_len)) {
                free(last);
                return VAGC_ERR_NO_MEMORY;
            }

            pos += best_len;
        } else {
            if (!vagc_buf_push_u8(out, 0)) {
                free(last);
                return VAGC_ERR_NO_MEMORY;
            }

            if (!vagc_buf_push_u8(out, in[pos])) {
                free(last);
                return VAGC_ERR_NO_MEMORY;
            }

            pos++;
        }
    }

    free(last);

    return VAGC_OK;
}

static vagc_error vagc_lzss_decode(const uint8_t* lz,
                                   size_t lz_len,
                                   size_t expected_size,
                                   uint8_t** out,
                                   size_t* out_size)
{
    vagc_buf b;
    size_t pos = 0;
    vagc_error err = VAGC_OK;

    *out = NULL;
    *out_size = 0;

    if (!vagc_buf_init(&b, expected_size)) {
        return VAGC_ERR_NO_MEMORY;
    }

    while (pos < lz_len) {
        uint8_t type = lz[pos++];

        if (type == 0) {
            if (pos >= lz_len) {
                err = VAGC_ERR_CORRUPT;
                goto fail;
            }

            if (b.len >= expected_size) {
                err = VAGC_ERR_CORRUPT;
                goto fail;
            }

            if (!vagc_buf_push_u8(&b, lz[pos++])) {
                err = VAGC_ERR_NO_MEMORY;
                goto fail;
            }
        } else if (type == 1) {
            if (pos + 4 > lz_len) {
                err = VAGC_ERR_CORRUPT;
                goto fail;
            }

            uint16_t offset16 = vagc_get_u16(lz + pos);
            uint16_t length16 = vagc_get_u16(lz + pos + 2);

            pos += 4;

            size_t offset = (size_t)offset16;
            size_t length = (size_t)length16;

            if (length == 0 || offset == 0) {
                err = VAGC_ERR_CORRUPT;
                goto fail;
            }

            if (offset > b.len) {
                err = VAGC_ERR_CORRUPT;
                goto fail;
            }

            if (length > expected_size - b.len) {
                err = VAGC_ERR_CORRUPT;
                goto fail;
            }

            if (!vagc_buf_reserve(&b, length)) {
                err = VAGC_ERR_NO_MEMORY;
                goto fail;
            }

            size_t src = b.len - offset;
            size_t i;

            for (i = 0; i < length; ++i) {
                b.data[b.len + i] = b.data[src + i];
            }

            b.len += length;
        } else {
            err = VAGC_ERR_CORRUPT;
            goto fail;
        }
    }

    if (b.len != expected_size) {
        err = VAGC_ERR_CORRUPT;
        goto fail;
    }

    *out = b.data;
    *out_size = b.len;

    return VAGC_OK;

fail:
    vagc_buf_free(&b);
    return err;
}

/* ------------------------------------------------------------------ */
/* Gzip via zlib                                                       */
/* ------------------------------------------------------------------ */

#ifdef VAG_USE_ZLIB

static vagc_error vagc_gzip_compress(const uint8_t* in,
                                     size_t in_size,
                                     uint8_t** out,
                                     size_t* out_size)
{
    z_stream strm;
    uint8_t dummy = 0;
    uint8_t* buf = NULL;
    uLong bound;
    int ret;

    *out = NULL;
    *out_size = 0;

#if ULONG_MAX > 0
    if ((unsigned long)in_size > ULONG_MAX) {
        return VAGC_ERR_TOO_LARGE;
    }
#endif

    memset(&strm, 0, sizeof(strm));

    ret = deflateInit2(&strm,
                       Z_DEFAULT_COMPRESSION,
                       Z_DEFLATED,
                       15 + 16, /* gzip wrapper */
                       8,
                       Z_DEFAULT_STRATEGY);

    if (ret != Z_OK) {
        return VAGC_ERR_NO_MEMORY;
    }

    bound = deflateBound(&strm, (uLong)in_size);

    if (bound == 0) {
        bound = 1;
    }

    if (bound > SIZE_MAX || bound > UINT_MAX) {
        deflateEnd(&strm);
        return VAGC_ERR_TOO_LARGE;
    }

    buf = (uint8_t*)malloc((size_t)bound);
    if (!buf) {
        deflateEnd(&strm);
        return VAGC_ERR_NO_MEMORY;
    }

    strm.next_in = (Bytef*)(in_size ? (const void*)in : (const void*)&dummy);
    strm.avail_in = (uInt)in_size;

    strm.next_out = buf;
    strm.avail_out = (uInt)bound;

    ret = deflate(&strm, Z_FINISH);

    if (ret != Z_STREAM_END) {
        free(buf);
        deflateEnd(&strm);
        return VAGC_ERR_IO;
    }

    *out = buf;
    *out_size = (size_t)strm.total_out;

    deflateEnd(&strm);

    return VAGC_OK;
}

static vagc_error vagc_gzip_decompress(const uint8_t* in,
                                       size_t in_size,
                                       size_t expected_size,
                                       uint8_t** out,
                                       size_t* out_size)
{
    z_stream strm;
    uint8_t* buf = NULL;
    size_t alloc_size;
    int ret;

    *out = NULL;
    *out_size = 0;

    if (in_size > UINT_MAX || expected_size > UINT_MAX) {
        return VAGC_ERR_TOO_LARGE;
    }

    memset(&strm, 0, sizeof(strm));

    ret = inflateInit2(&strm, 15 + 32); /* auto zlib/gzip */

    if (ret != Z_OK) {
        return VAGC_ERR_NO_MEMORY;
    }

    alloc_size = (expected_size > 0) ? expected_size : 1;

    buf = (uint8_t*)malloc(alloc_size);
    if (!buf) {
        inflateEnd(&strm);
        return VAGC_ERR_NO_MEMORY;
    }

    strm.next_in = (Bytef*)(const void*)in;
    strm.avail_in = (uInt)in_size;

    strm.next_out = buf;
    strm.avail_out = (uInt)alloc_size;

    ret = inflate(&strm, Z_FINISH);

    if (ret != Z_STREAM_END || strm.total_out != (uLong)expected_size) {
        free(buf);
        inflateEnd(&strm);
        return VAGC_ERR_CORRUPT;
    }

    inflateEnd(&strm);

    if (expected_size == 0) {
        free(buf);
        *out = NULL;
        *out_size = 0;
    } else {
        *out = buf;
        *out_size = expected_size;
    }

    return VAGC_OK;
}

#endif /* VAG_USE_ZLIB */

/* ------------------------------------------------------------------ */
/* Payload builders                                                    */
/* ------------------------------------------------------------------ */

static vagc_error vagc_make_none_payload(const uint8_t* in,
                                         size_t in_size,
                                         uint8_t** out,
                                         size_t* out_size)
{
    *out = NULL;
    *out_size = 0;

    if (in_size == 0) {
        return VAGC_OK;
    }

    if (!in) {
        return VAGC_ERR_NULL;
    }

    uint8_t* p = (uint8_t*)malloc(in_size);
    if (!p) {
        return VAGC_ERR_NO_MEMORY;
    }

    memcpy(p, in, in_size);

    *out = p;
    *out_size = in_size;

    return VAGC_OK;
}

static vagc_error vagc_make_huff_payload(const uint8_t* in,
                                         size_t in_size,
                                         uint8_t** out,
                                         size_t* out_size)
{
    return vagc_huffman_encode_payload(in, in_size, out, out_size);
}

static vagc_error vagc_make_lzhuff_payload(const uint8_t* in,
                                           size_t in_size,
                                           uint8_t** out,
                                           size_t* out_size)
{
    vagc_buf lz;
    uint8_t* huff_payload = NULL;
    size_t huff_size = 0;
    vagc_error err;

    *out = NULL;
    *out_size = 0;

    if (!vagc_buf_init(&lz, in_size / 2 + 16)) {
        return VAGC_ERR_NO_MEMORY;
    }

    err = vagc_lzss_encode(in, in_size, &lz);
    if (err != VAGC_OK) {
        vagc_buf_free(&lz);
        return err;
    }

    err = vagc_huffman_encode_payload(lz.data, lz.len, &huff_payload, &huff_size);
    vagc_buf_free(&lz);

    if (err != VAGC_OK) {
        return err;
    }

    if (huff_size > SIZE_MAX - 8) {
        free(huff_payload);
        return VAGC_ERR_TOO_LARGE;
    }

    size_t total = 8 + huff_size;
    uint8_t* payload = (uint8_t*)malloc(total);

    if (!payload) {
        free(huff_payload);
        return VAGC_ERR_NO_MEMORY;
    }

    vagc_put_u64(payload, (uint64_t)lz.len);

    if (huff_size > 0) {
        memcpy(payload + 8, huff_payload, huff_size);
    }

    free(huff_payload);

    *out = payload;
    *out_size = total;

    return VAGC_OK;
}

#ifdef VAG_USE_ZLIB

static vagc_error vagc_make_gzip_payload(const uint8_t* in,
                                         size_t in_size,
                                         uint8_t** out,
                                         size_t* out_size)
{
    return vagc_gzip_compress(in, in_size, out, out_size);
}

static vagc_error vagc_make_lzhuff_gzip_payload(const uint8_t* in,
                                                size_t in_size,
                                                uint8_t** out,
                                                size_t* out_size)
{
    uint8_t* lzh_payload = NULL;
    size_t lzh_size = 0;
    uint8_t* gz_payload = NULL;
    size_t gz_size = 0;
    vagc_error err;

    *out = NULL;
    *out_size = 0;

    err = vagc_make_lzhuff_payload(in, in_size, &lzh_payload, &lzh_size);
    if (err != VAGC_OK) {
        return err;
    }

    err = vagc_gzip_compress(lzh_payload, lzh_size, &gz_payload, &gz_size);
    if (err != VAGC_OK) {
        free(lzh_payload);
        return err;
    }

    if (gz_size > SIZE_MAX - 8) {
        free(lzh_payload);
        free(gz_payload);
        return VAGC_ERR_TOO_LARGE;
    }

    size_t total = 8 + gz_size;
    uint8_t* payload = (uint8_t*)malloc(total);

    if (!payload) {
        free(lzh_payload);
        free(gz_payload);
        return VAGC_ERR_NO_MEMORY;
    }

    vagc_put_u64(payload, (uint64_t)lzh_size);

    if (gz_size > 0) {
        memcpy(payload + 8, gz_payload, gz_size);
    }

    free(lzh_payload);
    free(gz_payload);

    *out = payload;
    *out_size = total;

    return VAGC_OK;
}

#endif /* VAG_USE_ZLIB */

/* ------------------------------------------------------------------ */
/* Payload decoders                                                    */
/* ------------------------------------------------------------------ */

static vagc_error vagc_decode_huff_payload(const uint8_t* payload,
                                           size_t payload_size,
                                           size_t original_size,
                                           uint8_t** out,
                                           size_t* out_size)
{
    return vagc_huffman_decode_payload(payload,
                                       payload_size,
                                       original_size,
                                       out,
                                       out_size);
}

static vagc_error vagc_decode_lzhuff_payload(const uint8_t* payload,
                                             size_t payload_size,
                                             size_t original_size,
                                             uint8_t** out,
                                             size_t* out_size)
{
    uint8_t* lz = NULL;
    size_t lz_len = 0;
    vagc_error err;

    *out = NULL;
    *out_size = 0;

    if (payload_size < 8) {
        return VAGC_ERR_CORRUPT;
    }

    uint64_t lz_len64 = vagc_get_u64(payload);

    if (lz_len64 > SIZE_MAX) {
        return VAGC_ERR_TOO_LARGE;
    }

    size_t symbol_count = (size_t)lz_len64;

    err = vagc_huffman_decode_payload(payload + 8,
                                      payload_size - 8,
                                      symbol_count,
                                      &lz,
                                      &lz_len);

    if (err != VAGC_OK) {
        return err;
    }

    err = vagc_lzss_decode(lz, lz_len, original_size, out, out_size);

    free(lz);

    return err;
}

/* ------------------------------------------------------------------ */
/* Public compress/decompress                                          */
/* ------------------------------------------------------------------ */

vagc_error vag_compress_data(vag_comp_method method,
                             const uint8_t* in,
                             size_t in_size,
                             uint8_t** out,
                             size_t* out_size)
{
    uint8_t* payload = NULL;
    size_t payload_size = 0;
    uint8_t actual_method = (uint8_t)method;
    vagc_error err = VAGC_OK;

    if (!out || !out_size) {
        return VAGC_ERR_NULL;
    }

    *out = NULL;
    *out_size = 0;

    if (in_size > 0 && !in) {
        return VAGC_ERR_NULL;
    }

    if (in_size > SIZE_MAX - VAG_CODEC_HEADER_SIZE) {
        return VAGC_ERR_TOO_LARGE;
    }

    switch (method) {
        case VAG_COMP_NONE:
            err = vagc_make_none_payload(in, in_size, &payload, &payload_size);
            break;

        case VAG_COMP_HUFF:
            err = vagc_make_huff_payload(in, in_size, &payload, &payload_size);
            break;

        case VAG_COMP_LZ_HUFF:
            err = vagc_make_lzhuff_payload(in, in_size, &payload, &payload_size);
            break;

#ifdef VAG_USE_ZLIB
        case VAG_COMP_GZIP:
            err = vagc_make_gzip_payload(in, in_size, &payload, &payload_size);
            break;

        case VAG_COMP_LZ_HUFF_GZIP:
            err = vagc_make_lzhuff_gzip_payload(in, in_size, &payload, &payload_size);
            break;
#else
        case VAG_COMP_GZIP:
        case VAG_COMP_LZ_HUFF_GZIP:
            err = VAGC_ERR_UNSUPPORTED;
            break;
#endif

        default:
            err = VAGC_ERR_UNSUPPORTED;
            break;
    }

    /*
     * 압축 실패 시 NONE fallback.
     * 단, UNSUPPORTED는 사용자에게 명시적으로 알린다.
     */
    if (err != VAGC_OK) {
        if (method == VAG_COMP_NONE || err == VAGC_ERR_UNSUPPORTED) {
            return err;
        }

        actual_method = VAG_COMP_NONE;
        err = vagc_make_none_payload(in, in_size, &payload, &payload_size);

        if (err != VAGC_OK) {
            return err;
        }
    }

    /*
     * 압축 결과가 무압축보다 커지면 NONE으로 fallback.
     */
    if (actual_method != VAG_COMP_NONE) {
        if (payload_size > SIZE_MAX - VAG_CODEC_HEADER_SIZE) {
            free(payload);
            return VAGC_ERR_TOO_LARGE;
        }

        size_t compressed_total = VAG_CODEC_HEADER_SIZE + payload_size;
        size_t none_total = VAG_CODEC_HEADER_SIZE + in_size;

        if (compressed_total > none_total) {
            free(payload);

            actual_method = VAG_COMP_NONE;

            err = vagc_make_none_payload(in, in_size, &payload, &payload_size);
            if (err != VAGC_OK) {
                return err;
            }
        }
    }

    if (payload_size > SIZE_MAX - VAG_CODEC_HEADER_SIZE) {
        free(payload);
        return VAGC_ERR_TOO_LARGE;
    }

    size_t total_size = VAG_CODEC_HEADER_SIZE + payload_size;

    uint8_t* blob = (uint8_t*)malloc(total_size);
    if (!blob) {
        free(payload);
        return VAGC_ERR_NO_MEMORY;
    }

    memcpy(blob, VAG_CODEC_MAGIC, VAG_CODEC_MAGIC_LEN);
    blob[4] = actual_method;
    vagc_put_u64(blob + 5, (uint64_t)in_size);

    if (payload_size > 0) {
        memcpy(blob + VAG_CODEC_HEADER_SIZE, payload, payload_size);
    }

    free(payload);

    *out = blob;
    *out_size = total_size;

    return VAGC_OK;
}

vagc_error vag_decompress_data(const uint8_t* blob,
                               size_t blob_size,
                               uint8_t** out,
                               size_t* out_size)
{
    *out = NULL;
    *out_size = 0;

    if (!blob || blob_size < VAG_CODEC_HEADER_SIZE) {
        return VAGC_ERR_CORRUPT;
    }

    if (memcmp(blob, VAG_CODEC_MAGIC, VAG_CODEC_MAGIC_LEN) != 0) {
        return VAGC_ERR_BAD_MAGIC;
    }

    uint8_t method = blob[4];
    uint64_t original_size64 = vagc_get_u64(blob + 5);

    if (original_size64 > SIZE_MAX) {
        return VAGC_ERR_TOO_LARGE;
    }

    size_t original_size = (size_t)original_size64;

    const uint8_t* payload = blob + VAG_CODEC_HEADER_SIZE;
    size_t payload_size = blob_size - VAG_CODEC_HEADER_SIZE;

    switch (method) {
        case VAG_COMP_NONE:
            if (payload_size != original_size) {
                return VAGC_ERR_CORRUPT;
            }

            if (original_size == 0) {
                *out = NULL;
                *out_size = 0;
                return VAGC_OK;
            }

            *out = (uint8_t*)malloc(original_size);
            if (!*out) {
                return VAGC_ERR_NO_MEMORY;
            }

            memcpy(*out, payload, original_size);
            *out_size = original_size;

            return VAGC_OK;

        case VAG_COMP_HUFF:
            return vagc_decode_huff_payload(payload,
                                            payload_size,
                                            original_size,
                                            out,
                                            out_size);

        case VAG_COMP_LZ_HUFF:
            return vagc_decode_lzhuff_payload(payload,
                                              payload_size,
                                              original_size,
                                              out,
                                              out_size);

#ifdef VAG_USE_ZLIB
        case VAG_COMP_GZIP:
            return vagc_gzip_decompress(payload,
                                        payload_size,
                                        original_size,
                                        out,
                                        out_size);

        case VAG_COMP_LZ_HUFF_GZIP: {
            if (payload_size < 8) {
                return VAGC_ERR_CORRUPT;
            }

            uint64_t inter_size64 = vagc_get_u64(payload);

            if (inter_size64 > SIZE_MAX) {
                return VAGC_ERR_TOO_LARGE;
            }

            size_t inter_size = (size_t)inter_size64;

            const uint8_t* gz = payload + 8;
            size_t gz_size = payload_size - 8;

            uint8_t* inter = NULL;
            size_t inter_len = 0;

            vagc_error err = vagc_gzip_decompress(gz,
                                                  gz_size,
                                                  inter_size,
                                                  &inter,
                                                  &inter_len);

            if (err != VAGC_OK) {
                return err;
            }

            err = vagc_decode_lzhuff_payload(inter,
                                             inter_len,
                                             original_size,
                                             out,
                                             out_size);

            free(inter);

            return err;
        }
#else
        case VAG_COMP_GZIP:
        case VAG_COMP_LZ_HUFF_GZIP:
            return VAGC_ERR_UNSUPPORTED;
#endif

        default:
            return VAGC_ERR_UNSUPPORTED;
    }
}

const char* vagc_strerror(vagc_error err)
{
    switch (err) {
        case VAGC_OK:
            return "성공";
        case VAGC_ERR_NULL:
            return "NULL 인자";
        case VAGC_ERR_NO_MEMORY:
            return "메모리 부족";
        case VAGC_ERR_CORRUPT:
            return "손상된 데이터";
        case VAGC_ERR_IO:
            return "입출력 오류";
        case VAGC_ERR_UNSUPPORTED:
            return "지원하지 않는 방식";
        case VAGC_ERR_TOO_LARGE:
            return "데이터가 너무 큼";
        case VAGC_ERR_BAD_MAGIC:
            return "잘못된 codec 매직";
        default:
            return "알 수 없는 오류";
    }
}

#ifdef __cplusplus
} /* extern "C" */
#endif
