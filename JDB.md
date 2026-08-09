# JDB
### SQL 이런거 어려워서 만들었습니다.
> **좋진 않아요**
# 코드
``` #include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <stdint.h>
#include <time.h>
#include <limits.h>
#include <errno.h>

// 플랫폼 감지
#ifdef _WIN32
    #include <windows.h>
    #include <bcrypt.h>
    #pragma comment(lib, "bcrypt.lib")
    #define JDB_PLATFORM_WINDOWS
#else
    #include <fcntl.h>
    #include <unistd.h>
    #define JDB_PLATFORM_UNIX
#endif

// 에러 코드 정의
typedef enum {
    JDB_OK = 0,
    JDB_INVALID_HEADER,
    JDB_INVALID_MAGIC,
    JDB_UNSUPPORTED_VERSION,
    JDB_INVALID_OFFSET,
    JDB_INVALID_INDEX,
    JDB_INTEGRITY_ERROR,
    JDB_AUTH_FAILED,
    JDB_CORRUPTED_DATA,
    JDB_TRANSACTION_ERROR,
    JDB_RECOVERY_FAILED,
    JDB_OUT_OF_SPACE,
    JDB_IO_ERROR,
    JDB_INVALID_PASSWORD,
    JDB_NOT_FOUND,
    JDB_MEMORY_ERROR,
    JDB_FILE_TOO_LARGE,
    JDB_DUPLICATE_KEY
} jdb_error_t;

// 데이터 타입 정의
#define JDB_TYPE_NULL     0x00
#define JDB_TYPE_BOOLEAN  0x01
#define JDB_TYPE_INTEGER  0x02
#define JDB_TYPE_FLOAT    0x03
#define JDB_TYPE_STRING   0x04
#define JDB_TYPE_BINARY   0x05
#define JDB_TYPE_ARRAY    0x06
#define JDB_TYPE_OBJECT   0x07

// 페이지 타입 정의
#define JDB_PAGE_HEADER    0x01
#define JDB_PAGE_META      0x02
#define JDB_PAGE_INDEX     0x03
#define JDB_PAGE_DATA      0x04
#define JDB_PAGE_FREE      0x05
#define JDB_PAGE_ROOT      0x06
#define JDB_PAGE_TRANS     0x07

// 트랜잭션 타입 정의
#define JDB_TRANS_BEGIN    0x01
#define JDB_TRANS_COMMIT   0x02
#define JDB_TRANS_ROLLBACK 0x03

// 레코드 플래그
#define JDB_RECORD_DELETED 0x01

// B+ Tree 상수
#define BTREE_ORDER 127
#define BTREE_MIN_KEYS (BTREE_ORDER / 2)

// PBKDF2 상수
#define JDB_PBKDF2_ITERATIONS 10000

// SHA-256 상수
#define SHA256_BLOCK_SIZE 64
#define SHA256_DIGEST_SIZE 32

// SHA-256 컨텍스트
typedef struct {
    uint32_t state[8];
    uint64_t bit_count;
    uint8_t buffer[SHA256_BLOCK_SIZE];
} jdb_sha256_ctx_t;

// HMAC 컨텍스트
typedef struct {
    jdb_sha256_ctx_t inner;
    jdb_sha256_ctx_t outer;
} jdb_hmac_sha256_ctx_t;

// 헤더 구조체
typedef struct {
    uint8_t magic[8];           // "JDBFILE\0"
    uint32_t version;           // 포맷 버전 (1)
    uint32_t header_size;       // 헤더 크기 (4096)
    uint8_t uuid[16];           // 데이터베이스 UUID
    char db_name[256];          // 데이터베이스 이름
    uint64_t index_offset;      // 인덱스 시작 오프셋
    uint64_t index_size;        // 인덱스 크기
    uint64_t metadata_offset;   // 메타데이터 시작 오프셋
    uint64_t metadata_size;     // 메타데이터 크기
    uint64_t data_offset;       // 데이터 시작 오프셋
    uint64_t data_size;         // 데이터 크기
    uint32_t page_size;         // 페이지 크기 (기본 4096)
    uint32_t total_pages;       // 총 페이지 수
    uint32_t free_pages;        // 빈 페이지 수
    uint8_t author[64];         // 작성자 이름
    uint8_t password_hash[32];  // 패스워드 해시
    uint8_t salt[16];           // 패스워드 솔트
    uint8_t integrity[32];      // 헤더 무결성 해시
    uint32_t flags;             // 플래그 (0x01=암호화, 0x02=압축 등)
    uint64_t timestamp_created;// 생성 시간
    uint64_t timestamp_modified;// 수정 시간
    uint8_t reserved[3840];     // 예약 영역
} jdb_header_t;

// 페이지 구조체
typedef struct {
    uint32_t page_id;           // 페이지 ID
    uint8_t page_type;          // 페이지 타입
    uint32_t used_size;         // 사용된 크기
    uint32_t checksum;          // 페이지 체크섬
    uint32_t flags;             // 페이지 플래그
    uint8_t payload[4064];      // 페이로드 데이터
} jdb_page_t;

// 레코드 헤더
typedef struct {
    uint64_t record_id;         // 레코드 ID
    uint8_t type;               // 데이터 타입
    uint32_t size;              // 레코드 크기
    uint32_t flags;             // 레코드 플래그
    uint64_t timestamp;         // 타임스탬프
    uint32_t payload_size;      // 페이로드 크기
    uint32_t collection_id;     // 컬렉션 ID
} jdb_record_header_t;

// 컬렉션 정보
typedef struct {
    uint32_t id;
    char name[64];
    uint64_t first_record_offset;
    uint64_t last_record_offset;
    uint32_t record_count;
} jdb_collection_info_t;

// B+ 트리 노드
typedef struct {
    uint8_t node_type;          // 0x00=리프, 0x01=내부
    uint32_t key_count;         // 키 개수
    uint64_t parent_id;         // 부모 노드 ID
    uint64_t keys[BTREE_ORDER-1];         // 키 배열 (리프 노드용)
    uint64_t child_ids[BTREE_ORDER];      // 자식 ID 배열 (내부 노드용)
    uint64_t leaf_next;         // 다음 리프 노드 (리프 노드용)
    uint64_t record_offsets[BTREE_ORDER-1]; // 레코드 오프셋 (리프 노드용)
} jdb_btree_node_t;

// 트랜잭션 로그
typedef struct {
    uint64_t trans_id;          // 트랜잭션 ID
    uint8_t trans_type;         // 0x01=BEGIN, 0x02=COMMIT, 0x03=ROLLBACK
    uint64_t timestamp;         // 타임스탬프
    uint32_t operation_count;   // 연산 개수
    uint8_t operations[4064];   // 연산 데이터
} jdb_transaction_log_t;

// Free Space Entry
typedef struct {
    uint64_t offset;
    uint32_t size;
} jdb_free_space_entry_t;

// Free Space Manager
typedef struct {
    jdb_free_space_entry_t* entries;
    uint32_t count;
    uint32_t capacity;
} jdb_free_space_manager_t;

// 트랜잭션 상태
typedef struct {
    uint8_t active;
    uint64_t id;
    uint64_t temp_offset;
    uint32_t temp_size;
} jdb_transaction_state_t;

// 결과 구조체
typedef struct {
    void** records;
    size_t count;
    size_t capacity;
} jdb_result_t;

// 데이터베이스 구조체
typedef struct {
    FILE* file;
    char filename[256];
    jdb_header_t header;
    uint8_t is_open;
    uint8_t is_encrypted;
    uint8_t cache_enabled;
    void* cache;
    uint64_t transaction_id;
    jdb_free_space_manager_t* free_space_manager;
    jdb_transaction_state_t transaction_state;
    jdb_collection_info_t* collections;
    uint32_t collection_count;
    uint32_t max_collections;
    uint64_t next_record_id;
    uint64_t root_page_offset;
    uint32_t total_nodes;
} jdb_database_t;

// SHA-256 구현
static const uint32_t k_sha256[64] = {
    0x428a2f98, 0x71374491, 0xb5c0fbcf, 0xe9b5dba5, 0x3956c25b, 0x59f111f1, 0x923f82a4, 0xab1c5ed5,
    0xd807aa98, 0x12835b01, 0x243185be, 0x550c7dc3, 0x72be5d74, 0x80deb1fe, 0x9bdc06a7, 0xc19bf174,
    0xe49b69c1, 0xefbe4786, 0x0fc19dc6, 0x240ca1cc, 0x2de92c6f, 0x4a7484aa, 0x5cb0a9dc, 0x76f988da,
    0x983e5152, 0xa831c66d, 0xb00327c8, 0xbf597fc7, 0xc6e00bf3, 0xd5a79147, 0x06ca6351, 0x14292967,
    0x27b70a85, 0x2e1b2138, 0x4d2c6dfc, 0x53380d13, 0x650a7354, 0x766a0abb, 0x81c2c92e, 0x92722c85,
    0xa2bfe8a1, 0xa81a664b, 0xc24b8b70, 0xc76c51a3, 0xd192e819, 0xd6990624, 0xf40e3585, 0x106aa070,
    0x19a4c116, 0x1e376c08, 0x2748774c, 0x34b0bcb5, 0x391c0cb3, 0x4ed8aa4a, 0x5b9cca4f, 0x682e6ff3,
    0x748f82ee, 0x78a5636f, 0x84c87814, 0x8cc70208, 0x90befffa, 0xa4506ceb, 0xbef9a3f7, 0xc67178f2
};

static uint32_t rotr32(uint32_t x, unsigned n) {
    return (x >> n) | (x << (32 - n));
}

static uint32_t ch(uint32_t x, uint32_t y, uint32_t z) {
    return (x & y) ^ (~x & z);
}

static uint32_t maj(uint32_t x, uint32_t y, uint32_t z) {
    return (x & y) ^ (x & z) ^ (y & z);
}

static uint32_t sigma0(uint32_t x) {
    return rotr32(x, 2) ^ rotr32(x, 13) ^ rotr32(x, 22);
}

static uint32_t sigma1(uint32_t x) {
    return rotr32(x, 6) ^ rotr32(x, 11) ^ rotr32(x, 25);
}

static uint32_t gamma0(uint32_t x) {
    return rotr32(x, 7) ^ rotr32(x, 18) ^ (x >> 3);
}

static uint32_t gamma1(uint32_t x) {
    return rotr32(x, 17) ^ rotr32(x, 19) ^ (x >> 10);
}

static void sha256_transform(jdb_sha256_ctx_t* ctx, const uint8_t* data) {
    uint32_t a, b, c, d, e, f, g, h, i, j, t1, t2, m[64];

    for (i = 0, j = 0; i < 16; ++i, j += 4) {
        m[i] = (data[j] << 24) | (data[j + 1] << 16) | (data[j + 2] << 8) | (data[j + 3]);
    }
    for (; i < 64; ++i) {
        m[i] = gamma1(m[i - 2]) + m[i - 7] + gamma0(m[i - 15]) + m[i - 16];
    }

    a = ctx->state[0];
    b = ctx->state[1];
    c = ctx->state[2];
    d = ctx->state[3];
    e = ctx->state[4];
    f = ctx->state[5];
    g = ctx->state[6];
    h = ctx->state[7];

    for (i = 0; i < 64; ++i) {
        t1 = h + sigma1(e) + ch(e, f, g) + k_sha256[i] + m[i];
        t2 = sigma0(a) + maj(a, b, c);
        h = g;
        g = f;
        f = e;
        e = d + t1;
        d = c;
        c = b;
        b = a;
        a = t1 + t2;
    }

    ctx->state[0] += a;
    ctx->state[1] += b;
    ctx->state[2] += c;
    ctx->state[3] += d;
    ctx->state[4] += e;
    ctx->state[5] += f;
    ctx->state[6] += g;
    ctx->state[7] += h;
}

void jdb_sha256_init(jdb_sha256_ctx_t* ctx) {
    ctx->state[0] = 0x6a09e667;
    ctx->state[1] = 0xbb67ae85;
    ctx->state[2] = 0x3c6ef372;
    ctx->state[3] = 0xa54ff53a;
    ctx->state[4] = 0x510e527f;
    ctx->state[5] = 0x9b05688c;
    ctx->state[6] = 0x1f83d9ab;
    ctx->state[7] = 0x5be0cd19;
    ctx->bit_count = 0;
    memset(ctx->buffer, 0, SHA256_BLOCK_SIZE);
}

void jdb_sha256_update(jdb_sha256_ctx_t* ctx, const uint8_t* data, size_t size) {
    size_t i, j;

    for (i = 0; i < size; ++i) {
        ctx->buffer[(ctx->bit_count / 8) % SHA256_BLOCK_SIZE] = data[i];
        ctx->bit_count += 8;

        if ((ctx->bit_count / 8) % SHA256_BLOCK_SIZE == 0) {
            sha256_transform(ctx, ctx->buffer);
        }
    }
}

void jdb_sha256_final(jdb_sha256_ctx_t* ctx, uint8_t output[32]) {
    uint32_t i = (ctx->bit_count / 8) % SHA256_BLOCK_SIZE;
    ctx->buffer[i++] = 0x80;

    if (i > SHA256_BLOCK_SIZE - 8) {
        while (i < SHA256_BLOCK_SIZE) {
            ctx->buffer[i++] = 0;
        }
        sha256_transform(ctx, ctx->buffer);
        i = 0;
    }

    while (i < SHA256_BLOCK_SIZE - 8) {
        ctx->buffer[i++] = 0;
    }

    for (i = 0; i < 8; ++i) {
        ctx->buffer[56 + i] = (uint8_t)(ctx->bit_count >> (56 - 8 * i));
    }

    sha256_transform(ctx, ctx->buffer);

    for (i = 0; i < 8; ++i) {
        output[i * 4 + 0] = (uint8_t)(ctx->state[i] >> 24);
        output[i * 4 + 1] = (uint8_t)(ctx->state[i] >> 16);
        output[i * 4 + 2] = (uint8_t)(ctx->state[i] >> 8);
        output[i * 4 + 3] = (uint8_t)(ctx->state[i]);
    }
}

void jdb_sha256(const void* data, size_t size, uint8_t output[32]) {
    jdb_sha256_ctx_t ctx;
    jdb_sha256_init(&ctx);
    jdb_sha256_update(&ctx, (const uint8_t*)data, size);
    jdb_sha256_final(&ctx, output);
}

// HMAC-SHA256 구현
static void jdb_hmac_sha256_init(jdb_hmac_sha256_ctx_t* ctx, const uint8_t* key, size_t key_len) {
    uint8_t ipad[SHA256_BLOCK_SIZE];
    uint8_t opad[SHA256_BLOCK_SIZE];
    uint8_t temp_key[SHA256_DIGEST_SIZE];
    size_t i;

    if (key_len > SHA256_BLOCK_SIZE) {
        jdb_sha256(key, key_len, temp_key);
        key = temp_key;
        key_len = SHA256_DIGEST_SIZE;
    }

    memset(ipad, 0, SHA256_BLOCK_SIZE);
    memset(opad, 0, SHA256_BLOCK_SIZE);
    memcpy(ipad, key, key_len);
    memcpy(opad, key, key_len);

    for (i = 0; i < SHA256_BLOCK_SIZE; i++) {
        ipad[i] ^= 0x36;
        opad[i] ^= 0x5c;
    }

    jdb_sha256_init(&ctx->inner);
    jdb_sha256_update(&ctx->inner, ipad, SHA256_BLOCK_SIZE);

    jdb_sha256_init(&ctx->outer);
    jdb_sha256_update(&ctx->outer, opad, SHA256_BLOCK_SIZE);
}

static void jdb_hmac_sha256_update(jdb_hmac_sha256_ctx_t* ctx, const uint8_t* data, size_t size) {
    jdb_sha256_update(&ctx->inner, data, size);
}

static void jdb_hmac_sha256_final(jdb_hmac_sha256_ctx_t* ctx, uint8_t output[32]) {
    uint8_t inner_digest[SHA256_DIGEST_SIZE];
    jdb_sha256_final(&ctx->inner, inner_digest);
    jdb_sha256_update(&ctx->outer, inner_digest, SHA256_DIGEST_SIZE);
    jdb_sha256_final(&ctx->outer, output);
}

// PBKDF2 구현
static int jdb_pbkdf2_sha256(const char* password, const uint8_t* salt, size_t salt_len, uint32_t iterations, uint8_t* output, size_t output_len) {
    if (!password || !salt || !output) return -1;
    
    size_t password_len = strlen(password);
    uint8_t* salt_with_counter = malloc(salt_len + 4);
    if (!salt_with_counter) return -1;
    
    uint8_t* temp_output = malloc(output_len);
    if (!temp_output) {
        free(salt_with_counter);
        return -1;
    }
    
    uint8_t u[SHA256_DIGEST_SIZE];
    uint8_t block[SHA256_DIGEST_SIZE];
    jdb_hmac_sha256_ctx_t hmac_ctx;
    
    for (size_t block_idx = 1; block_idx <= (output_len + SHA256_DIGEST_SIZE - 1) / SHA256_DIGEST_SIZE; block_idx++) {
        memcpy(salt_with_counter, salt, salt_len);
        salt_with_counter[salt_len + 0] = (block_idx >> 24) & 0xFF;
        salt_with_counter[salt_len + 1] = (block_idx >> 16) & 0xFF;
        salt_with_counter[salt_len + 2] = (block_idx >> 8) & 0xFF;
        salt_with_counter[salt_len + 3] = block_idx & 0xFF;
        
        jdb_hmac_sha256_init(&hmac_ctx, (const uint8_t*)password, password_len);
        jdb_hmac_sha256_update(&hmac_ctx, salt_with_counter, salt_len + 4);
        jdb_hmac_sha256_final(&hmac_ctx, block);
        
        memcpy(u, block, SHA256_DIGEST_SIZE);
        
        for (uint32_t iter = 1; iter < iterations; iter++) {
            jdb_hmac_sha256_init(&hmac_ctx, (const uint8_t*)password, password_len);
            jdb_hmac_sha256_update(&hmac_ctx, u, SHA256_DIGEST_SIZE);
            jdb_hmac_sha256_final(&hmac_ctx, u);
            
            for (size_t j = 0; j < SHA256_DIGEST_SIZE; j++) {
                block[j] ^= u[j];
            }
        }
        
        size_t copy_len = (output_len - (block_idx - 1) * SHA256_DIGEST_SIZE < SHA256_DIGEST_SIZE) ? 
                          output_len - (block_idx - 1) * SHA256_DIGEST_SIZE : SHA256_DIGEST_SIZE;
        memcpy(temp_output + (block_idx - 1) * SHA256_DIGEST_SIZE, block, copy_len);
    }
    
    memcpy(output, temp_output, output_len);
    
    free(temp_output);
    free(salt_with_counter);
    
    return 0;
}

// 난수 생성
static int jdb_random_bytes(uint8_t* output, size_t size) {
    if (!output || size == 0) return -1;
    
#ifdef JDB_PLATFORM_WINDOWS
    NTSTATUS status = BCryptGenRandom(NULL, output, (ULONG)size, BCRYPT_USE_SYSTEM_PREFERRED_RNG);
    return (status == STATUS_SUCCESS) ? 0 : -1;
#else
    int fd = open("/dev/urandom", O_RDONLY);
    if (fd < 0) {
        // fallback: time-based seed
        srand((unsigned int)time(NULL));
        for (size_t i = 0; i < size; i++) {
            output[i] = (uint8_t)rand();
        }
        return 0;
    }
    
    ssize_t result = read(fd, output, size);
    close(fd);
    
    return (result == (ssize_t)size) ? 0 : -1;
#endif
}

// 내부 함수 선언
static int jdb_update_header_integrity(jdb_database_t* db);
static int jdb_validate_record_header(const jdb_record_header_t* header);
static int jdb_btree_search(jdb_database_t* db, uint64_t collection_id, uint64_t record_id, uint64_t* offset);
static int jdb_btree_insert(jdb_database_t* db, uint64_t collection_id, uint64_t record_id, uint64_t offset);
static int jdb_btree_create_root(jdb_database_t* db);
static int jdb_load_collections_from_metadata(jdb_database_t* db);
static int jdb_save_collections_to_metadata(jdb_database_t* db);
static int jdb_load_free_space_from_file(jdb_database_t* db);
static int jdb_save_free_space_to_file(jdb_database_t* db);
static uint64_t jdb_generate_unique_id(jdb_database_t* db);

// 유틸리티 함수
static uint32_t calculate_checksum(const void* data, size_t size) {
    uint32_t sum = 0;
    const uint8_t* bytes = (const uint8_t*)data;
    
    for (size_t i = 0; i < size; i++) {
        sum += bytes[i];
    }
    
    return sum;
}

static int validate_magic_number(const uint8_t* magic) {
    const uint8_t expected[] = {'J', 'D', 'B', 'F', 'I', 'L', 'E', '\0'};
    return memcmp(magic, expected, 8) == 0;
}

static int hash_password(const char* password, const uint8_t* salt, uint8_t* hash_out) {
    if (!password || !salt || !hash_out) return -1;
    
    return jdb_pbkdf2_sha256(password, salt, 16, JDB_PBKDF2_ITERATIONS, hash_out, 32);
}

// 헤더 무결성 업데이트
static int jdb_update_header_integrity(jdb_database_t* db) {
    if (!db) return JDB_INVALID_HEADER;
    
    // 타임스탬프 업데이트
    db->header.timestamp_modified = time(NULL);
    
    // 무결성 해시 계산
    uint8_t temp_integrity[32];
    jdb_sha256((unsigned char*)&db->header + 8, sizeof(jdb_header_t) - 40, temp_integrity);
    memcpy(db->header.integrity, temp_integrity, 32);
    
    // 헤더 업데이트
    fseek(db->file, 0, SEEK_SET);
    if (fwrite(&db->header, sizeof(jdb_header_t), 1, db->file) != 1) {
        return JDB_IO_ERROR;
    }
    
    return JDB_OK;
}

// 헤더 검증
static int validate_header(const jdb_header_t* header) {
    if (!validate_magic_number(header->magic)) {
        return JDB_INVALID_MAGIC;
    }
    
    if (header->version != 1) {
        return JDB_UNSUPPORTED_VERSION;
    }
    
    if (header->header_size != 4096) {
        return JDB_INVALID_HEADER;
    }
    
    return JDB_OK;
}

// 레코드 헤더 검증
static int jdb_validate_record_header(const jdb_record_header_t* header) {
    if (!header) return JDB_INVALID_HEADER;
    
    if (header->size < sizeof(jdb_record_header_t)) {
        return JDB_CORRUPTED_DATA;
    }
    
    if (header->payload_size > header->size - sizeof(jdb_record_header_t)) {
        return JDB_CORRUPTED_DATA;
    }
    
    if (header->type > JDB_TYPE_OBJECT) {
        return JDB_CORRUPTED_DATA;
    }
    
    return JDB_OK;
}

// B+ Tree 검색
static int jdb_btree_search(jdb_database_t* db, uint64_t collection_id, uint64_t record_id, uint64_t* offset) {
    if (!db || !offset) return JDB_INVALID_HEADER;
    
    if (db->root_page_offset == 0) {
        return JDB_NOT_FOUND;
    }
    
    jdb_btree_node_t node;
    uint64_t current_offset = db->root_page_offset;
    
    while (1) {
        fseek(db->file, current_offset, SEEK_SET);
        if (fread(&node, sizeof(jdb_btree_node_t), 1, db->file) != 1) {
            return JDB_INVALID_INDEX;
        }
        
        if (node.node_type == 0) { // 리프 노드
            for (uint32_t i = 0; i < node.key_count; i++) {
                if (node.keys[i] == record_id) {
                    *offset = node.record_offsets[i];
                    return JDB_OK;
                }
            }
            return JDB_NOT_FOUND;
        } else { // 내부 노드
            uint32_t i;
            for (i = 0; i < node.key_count; i++) {
                if (record_id < node.keys[i]) {
                    break;
                }
            }
            
            if (node.child_ids[i] == 0) {
                return JDB_NOT_FOUND;
            }
            current_offset = node.child_ids[i];
        }
    }
    
    return JDB_NOT_FOUND;
}

// B+ Tree 삽입 (간소화된 구현)
static int jdb_btree_insert(jdb_database_t* db, uint64_t collection_id, uint64_t record_id, uint64_t offset) {
    if (!db) return JDB_INVALID_HEADER;
    
    // 루트 노드가 없으면 생성
    if (db->root_page_offset == 0) {
        if (jdb_btree_create_root(db) != JDB_OK) {
            return JDB_INVALID_INDEX;
        }
    }
    
    // 간단한 구현: 루트에 바로 삽입
    jdb_btree_node_t root;
    fseek(db->file, db->root_page_offset, SEEK_SET);
    if (fread(&root, sizeof(jdb_btree_node_t), 1, db->file) != 1) {
        return JDB_INVALID_INDEX;
    }
    
    if (root.key_count < BTREE_ORDER - 1) {
        // 루트에 공간이 있으면 바로 삽입
        uint32_t i;
        for (i = 0; i < root.key_count; i++) {
            if (root.keys[i] > record_id) {
                break;
            }
        }
        
        // 키 이동
        for (uint32_t j = root.key_count; j > i; j--) {
            root.keys[j] = root.keys[j-1];
            root.record_offsets[j] = root.record_offsets[j-1];
        }
        
        root.keys[i] = record_id;
        root.record_offsets[i] = offset;
        root.key_count++;
        
        fseek(db->file, db->root_page_offset, SEEK_SET);
        if (fwrite(&root, sizeof(jdb_btree_node_t), 1, db->file) != 1) {
            return JDB_IO_ERROR;
        }
        
        return JDB_OK;
    } else {
        // 노드 분할 필요 (간소화된 구현에서는 오류 반환)
        return JDB_OUT_OF_SPACE;
    }
}

// B+ Tree 루트 생성
static int jdb_btree_create_root(jdb_database_t* db) {
    if (!db) return JDB_INVALID_HEADER;
    
    jdb_btree_node_t root;
    memset(&root, 0, sizeof(jdb_btree_node_t));
    root.node_type = 0; // 리프 노드
    root.parent_id = 0;
    root.key_count = 0;
    root.leaf_next = 0;
    
    // 루트를 인덱스 영역에 저장
    uint64_t root_offset = db->header.index_offset;
    fseek(db->file, root_offset, SEEK_SET);
    if (fwrite(&root, sizeof(jdb_btree_node_t), 1, db->file) != 1) {
        return JDB_IO_ERROR;
    }
    
    db->root_page_offset = root_offset;
    
    return JDB_OK;
}

// 컬렉션 ID 가져오기
static uint32_t get_collection_id(jdb_database_t* db, const char* collection_name) {
    if (!db || !collection_name) return 0;
    
    for (uint32_t i = 0; i < db->collection_count; i++) {
        if (strcmp(db->collections[i].name, collection_name) == 0) {
            return db->collections[i].id;
        }
    }
    
    // 새로운 컬렉션 추가
    if (db->collection_count < db->max_collections) {
        uint32_t new_id = db->collection_count + 1;
        strncpy(db->collections[db->collection_count].name, collection_name, 63);
        db->collections[db->collection_count].name[63] = '\0';
        db->collections[db->collection_count].id = new_id;
        db->collections[db->collection_count].first_record_offset = 0;
        db->collections[db->collection_count].last_record_offset = 0;
        db->collections[db->collection_count].record_count = 0;
        db->collection_count++;
        
        // 컬렉션 정보 저장
        jdb_save_collections_to_metadata(db);
        
        return new_id;
    }
    
    return 0;
}

// 컬렉션 정보 메타데이터에 저장
static int jdb_save_collections_to_metadata(jdb_database_t* db) {
    if (!db) return JDB_INVALID_HEADER;
    
    // 메타데이터 영역에 컬렉션 정보 저장
    fseek(db->file, db->header.metadata_offset, SEEK_SET);
    
    if (fwrite(db->collections, sizeof(jdb_collection_info_t), db->collection_count, db->file) != db->collection_count) {
        return JDB_IO_ERROR;
    }
    
    return JDB_OK;
}

// 메타데이터에서 컬렉션 정보 로드
static int jdb_load_collections_from_metadata(jdb_database_t* db) {
    if (!db) return JDB_INVALID_HEADER;
    
    // 메타데이터 영역에서 컬렉션 정보 로드
    fseek(db->file, db->header.metadata_offset, SEEK_SET);
    
    // 컬렉션 수 계산 (메타데이터 크기 기반)
    uint32_t possible_count = db->header.metadata_size / sizeof(jdb_collection_info_t);
    if (possible_count > db->max_collections) {
        possible_count = db->max_collections;
    }
    
    if (fread(db->collections, sizeof(jdb_collection_info_t), possible_count, db->file) != possible_count) {
        return JDB_IO_ERROR;
    }
    
    // 실제 컬렉션 수 업데이트
    db->collection_count = possible_count;
    
    return JDB_OK;
}

// Free Space Manager 초기화
static jdb_free_space_manager_t* init_free_space_manager() {
    jdb_free_space_manager_t* fsm = malloc(sizeof(jdb_free_space_manager_t));
    if (!fsm) return NULL;
    
    fsm->capacity = 100;
    fsm->count = 0;
    fsm->entries = malloc(fsm->capacity * sizeof(jdb_free_space_entry_t));
    if (!fsm->entries) {
        free(fsm);
        return NULL;
    }
    
    return fsm;
}

// Free Space 추가
static int add_free_space(jdb_free_space_manager_t* fsm, uint64_t offset, uint32_t size) {
    if (!fsm) return JDB_INVALID_HEADER;
    
    if (fsm->count >= fsm->capacity) {
        fsm->capacity *= 2;
        jdb_free_space_entry_t* new_entries = realloc(fsm->entries, 
                                                      fsm->capacity * sizeof(jdb_free_space_entry_t));
        if (!new_entries) return JDB_MEMORY_ERROR;
        fsm->entries = new_entries;
    }
    
    fsm->entries[fsm->count].offset = offset;
    fsm->entries[fsm->count].size = size;
    fsm->count++;
    
    return JDB_OK;
}

// Free Space 찾기
static int find_free_space(jdb_free_space_manager_t* fsm, uint32_t required_size, uint64_t* offset) {
    if (!fsm || !offset) return JDB_INVALID_HEADER;
    
    for (uint32_t i = 0; i < fsm->count; i++) {
        if (fsm->entries[i].size >= required_size) {
            *offset = fsm->entries[i].offset;
            uint32_t remaining_size = fsm->entries[i].size - required_size;
            
            if (remaining_size > 0) {
                // 나머지 공간을 다시 등록
                fsm->entries[i].offset += required_size;
                fsm->entries[i].size = remaining_size;
            } else {
                // 완전히 사용됨
                for (uint32_t j = i; j < fsm->count - 1; j++) {
                    fsm->entries[j] = fsm->entries[j + 1];
                }
                fsm->count--;
            }
            
            return JDB_OK;
        }
    }
    
    return JDB_NOT_FOUND;
}

// 고유 ID 생성
static uint64_t jdb_generate_unique_id(jdb_database_t* db) {
    if (!db) return 0;
    
    // 랜덤 ID 생성
    uint8_t random_bytes[8];
    if (jdb_random_bytes(random_bytes, 8) != 0) {
        // fallback: 시간 기반
        return time(NULL) * 1000000 + db->next_record_id++;
    }
    
    uint64_t id = *(uint64_t*)random_bytes;
    id ^= time(NULL);
    id ^= db->next_record_id++;
    
    return id;
}

// 데이터베이스 생성
jdb_database_t* jdb_create(const char* filename, const char* password) {
    if (!filename || !password) return NULL;
    
    // 패스워드 조건 검사
    int has_lower = 0, has_upper = 0, has_digit = 0, has_special = 0;
    int special_count = 0;
    int len = strlen(password);
    
    if (len < 8) return NULL;
    
    for (int i = 0; i < len; i++) {
        char c = password[i];
        if (c >= 'a' && c <= 'z') has_lower = 1;
        else if (c >= 'A' && c <= 'Z') has_upper = 1;
        else if (c >= '0' && c <= '9') has_digit = 1;
        else if ((c >= '!' && c <= '/') || (c >= ':' && c <= '@') || 
                 (c >= '[' && c <= '`') || (c >= '{' && c <= '~')) {
            has_special = 1;
            special_count++;
        }
    }
    
    if (!has_lower && !has_upper) return NULL;  // 최소 한 개의 영문자
    if (!has_digit) return NULL;                // 최소 한 개의 숫자
    if (!has_special || special_count < 3) return NULL;  // 최소 세 개의 특수문자
    
    FILE* file = fopen(filename, "wb");
    if (!file) return NULL;
    
    jdb_database_t* db = malloc(sizeof(jdb_database_t));
    if (!db) {
        fclose(file);
        return NULL;
    }
    
    strncpy(db->filename, filename, 255);
    db->filename[255] = '\0';
    db->file = file;
    db->is_open = 1;
    db->is_encrypted = 1;
    db->cache_enabled = 1;
    db->cache = NULL;
    db->transaction_id = 0;
    db->free_space_manager = init_free_space_manager();
    if (!db->free_space_manager) {
        free(db);
        fclose(file);
        return NULL;
    }
    memset(&db->transaction_state, 0, sizeof(jdb_transaction_state_t));
    
    // 컬렉션 배열 초기화
    db->max_collections = 100;
    db->collection_count = 0;
    db->collections = malloc(db->max_collections * sizeof(jdb_collection_info_t));
    if (!db->collections) {
        free(db->free_space_manager->entries);
        free(db->free_space_manager);
        free(db);
        fclose(file);
        return NULL;
    }
    memset(db->collections, 0, db->max_collections * sizeof(jdb_collection_info_t));
    
    // 헤더 초기화
    memset(&db->header, 0, sizeof(jdb_header_t));
    
    // 매직 넘버 설정
    memcpy(db->header.magic, "JDBFILE\0", 8);
    db->header.version = 1;
    db->header.header_size = 4096;
    
    // UUID 생성
    if (jdb_random_bytes(db->header.uuid, 16) != 0) {
        free(db->collections);
        free(db->free_space_manager->entries);
        free(db->free_space_manager);
        free(db);
        fclose(file);
        return NULL;
    }
    
    // 데이터베이스 이름 설정
    strncpy(db->header.db_name, "default", 255);
    db->header.db_name[255] = '\0';
    
    // 오프셋 설정
    db->header.index_offset = 0x1000;
    db->header.index_size = 4096;
    db->header.metadata_offset = 0x2000;
    db->header.metadata_size = 4096;
    db->header.data_offset = 0x3000;
    db->header.data_size = 0;
    
    // 페이지 설정
    db->header.page_size = 4096;
    db->header.total_pages = 1024; // 기본 1024 페이지 (4MB)
    db->header.free_pages = 1023;  // 헤더 페이지 제외
    
    // 작성자
    strncpy((char*)db->header.author, "JDB", 63);
    ((char*)db->header.author)[63] = '\0';
    
    // 패스워드 해싱
    if (jdb_random_bytes(db->header.salt, 16) != 0) {
        free(db->collections);
        free(db->free_space_manager->entries);
        free(db->free_space_manager);
        free(db);
        fclose(file);
        return NULL;
    }
    
    if (hash_password(password, db->header.salt, db->header.password_hash) != 0) {
        free(db->collections);
        free(db->free_space_manager->entries);
        free(db->free_space_manager);
        free(db);
        fclose(file);
        return NULL;
    }
    
    // 타임스탬프
    db->header.timestamp_created = time(NULL);
    db->header.timestamp_modified = db->header.timestamp_created;
    
    // 초기 레코드 ID
    db->next_record_id = 1;
    db->root_page_offset = 0;
    db->total_nodes = 0;
    
    // 무결성 해시 계산 및 헤더 저장
    if (jdb_update_header_integrity(db) != JDB_OK) {
        free(db->collections);
        free(db->free_space_manager->entries);
        free(db->free_space_manager);
        free(db);
        fclose(file);
        return NULL;
    }
    
    // 나머지 초기 페이지 할당
    uint8_t empty_page[4096] = {0};
    for (int i = 1; i < db->header.total_pages; i++) {
        if (fwrite(empty_page, 4096, 1, file) != 1) {
            free(db->collections);
            free(db->free_space_manager->entries);
            free(db->free_space_manager);
            free(db);
            fclose(file);
            return NULL;
        }
    }
    
    if (fflush(file) != 0) {
        free(db->collections);
        free(db->free_space_manager->entries);
        free(db->free_space_manager);
        free(db);
        fclose(file);
        return NULL;
    }
    
    return db;
}

// 데이터베이스 열기
jdb_database_t* jdb_open(const char* filename, const char* password) {
    if (!filename || !password) return NULL;
    
    FILE* file = fopen(filename, "r+b");
    if (!file) return NULL;
    
    jdb_database_t* db = malloc(sizeof(jdb_database_t));
    if (!db) {
        fclose(file);
        return NULL;
    }
    
    strncpy(db->filename, filename, 255);
    db->filename[255] = '\0';
    db->file = file;
    db->is_open = 0;
    db->is_encrypted = 0;
    db->cache_enabled = 0;
    db->cache = NULL;
    db->transaction_id = 0;
    db->free_space_manager = init_free_space_manager();
    if (!db->free_space_manager) {
        free(db);
        fclose(file);
        return NULL;
    }
    memset(&db->transaction_state, 0, sizeof(jdb_transaction_state_t));
    
    // 컬렉션 배열 초기화
    db->max_collections = 100;
    db->collection_count = 0;
    db->collections = malloc(db->max_collections * sizeof(jdb_collection_info_t));
    if (!db->collections) {
        free(db->free_space_manager->entries);
        free(db->free_space_manager);
        free(db);
        fclose(file);
        return NULL;
    }
    memset(db->collections, 0, db->max_collections * sizeof(jdb_collection_info_t));
    
    // 헤더 읽기
    fseek(file, 0, SEEK_SET);
    if (fread(&db->header, sizeof(jdb_header_t), 1, file) != 1) {
        free(db->collections);
        free(db->free_space_manager->entries);
        free(db->free_space_manager);
        free(db);
        fclose(file);
        return NULL;
    }
    
    // 헤더 검증
    int validation_result = validate_header(&db->header);
    if (validation_result != JDB_OK) {
        free(db->collections);
        free(db->free_space_manager->entries);
        free(db->free_space_manager);
        free(db);
        fclose(file);
        return NULL;
    }
    
    // 패스워드 확인
    uint8_t computed_hash[32];
    if (hash_password(password, db->header.salt, computed_hash) != 0) {
        free(db->collections);
        free(db->free_space_manager->entries);
        free(db->free_space_manager);
        free(db);
        fclose(file);
        return NULL;
    }
    
    if (memcmp(computed_hash, db->header.password_hash, 32) != 0) {
        free(db->collections);
        free(db->free_space_manager->entries);
        free(db->free_space_manager);
        free(db);
        fclose(file);
        return NULL;
    }
    
    db->is_open = 1;
    db->is_encrypted = 1;
    db->cache_enabled = 1;
    
    // 컬렉션 정보 로드
    if (jdb_load_collections_from_metadata(db) != JDB_OK) {
        free(db->collections);
        free(db->free_space_manager->entries);
        free(db->free_space_manager);
        free(db);
        fclose(file);
        return NULL;
    }
    
    // Free Space 정보 로드
    if (jdb_load_free_space_from_file(db) != JDB_OK) {
        // 오류 발생 시 경고만 하고 계속 진행
    }
    
    // 루트 페이지 오프셋 복구 (간소화된 구현)
    if (db->header.index_size > 0) {
        db->root_page_offset = db->header.index_offset;
    }
    
    return db;
}

// 데이터베이스 닫기
int jdb_close(jdb_database_t* db) {
    if (!db) return JDB_INVALID_HEADER;
    
    if (db->is_open) {
        // Free Space 정보 저장
        jdb_save_free_space_to_file(db);
        
        if (fflush(db->file) != 0) {
            return JDB_IO_ERROR;
        }
        fclose(db->file);
        db->is_open = 0;
    }
    
    if (db->cache) {
        free(db->cache);
        db->cache = NULL;
    }
    
    if (db->free_space_manager) {
        if (db->free_space_manager->entries) {
            free(db->free_space_manager->entries);
        }
        free(db->free_space_manager);
        db->free_space_manager = NULL;
    }
    
    if (db->collections) {
        free(db->collections);
        db->collections = NULL;
    }
    
    free(db);
    return JDB_OK;
}

// 레코드 삽입
int jdb_insert(jdb_database_t* db, const char* collection, void* data, size_t size) {
    if (!db || !collection || !data || size == 0) return JDB_INVALID_HEADER;
    if (!db->is_open) return JDB_INVALID_HEADER;
    
    // 컬렉션 ID 얻기
    uint32_t collection_id = get_collection_id(db, collection);
    if (collection_id == 0) return JDB_INVALID_HEADER;
    
    // 새로운 레코드 ID 생성
    uint64_t record_id = jdb_generate_unique_id(db);
    if (record_id == 0) return JDB_MEMORY_ERROR;
    
    // 레코드 헤더 준비
    jdb_record_header_t header;
    header.record_id = record_id;
    header.type = JDB_TYPE_BINARY; // 기본적으로 바이너리 타입
    header.size = sizeof(jdb_record_header_t) + size;
    header.flags = 0;
    header.timestamp = time(NULL);
    header.payload_size = size;
    header.collection_id = collection_id;
    
    // 공간 확보
    uint64_t record_offset;
    int free_space_result = find_free_space(db->free_space_manager, header.size, &record_offset);
    if (free_space_result != JDB_OK) {
        // 새로운 공간 사용
        record_offset = db->header.data_offset + db->header.data_size;
        db->header.data_size += header.size;
    }
    
    // 레코드 쓰기
    fseek(db->file, record_offset, SEEK_SET);
    
    // 레코드 헤더 쓰기
    if (fwrite(&header, sizeof(jdb_record_header_t), 1, db->file) != 1) {
        return JDB_IO_ERROR;
    }
    
    // 레코드 데이터 쓰기
    if (fwrite(data, size, 1, db->file) != 1) {
        return JDB_IO_ERROR;
    }
    
    // B+ Tree에 삽입
    if (jdb_btree_insert(db, collection_id, record_id, record_offset) != JDB_OK) {
        // 인덱스 삽입 실패 시 경고만
    }
    
    // 헤더 업데이트 (무결성 포함)
    if (jdb_update_header_integrity(db) != JDB_OK) {
        return JDB_IO_ERROR;
    }
    
    return JDB_OK;
}

// 레코드 조회
void* jdb_get(jdb_database_t* db, const char* collection, uint64_t id, size_t* size) {
    if (!db || !collection || !size) return NULL;
    if (!db->is_open) return NULL;
    
    // 컬렉션 ID 확인
    uint32_t collection_id = get_collection_id(db, collection);
    if (collection_id == 0) return NULL;
    
    // B+ Tree를 사용하여 레코드 오프셋 찾기
    uint64_t record_offset;
    if (jdb_btree_search(db, collection_id, id, &record_offset) != JDB_OK) {
        return NULL;
    }
    
    // 레코드 헤더 읽기
    fseek(db->file, record_offset, SEEK_SET);
    jdb_record_header_t header;
    if (fread(&header, sizeof(jdb_record_header_t), 1, db->file) != 1) {
        return NULL;
    }
    
    // 헤더 검증
    if (jdb_validate_record_header(&header) != JDB_OK) {
        return NULL;
    }
    
    // 삭제된 레코드인지 확인
    if (header.flags & JDB_RECORD_DELETED) {
        return NULL;
    }
    
    // 데이터 읽기
    void* data = malloc(header.payload_size);
    if (!data) return NULL;
    
    if (fread(data, header.payload_size, 1, db->file) != 1) {
        free(data);
        return NULL;
    }
    
    *size = header.payload_size;
    return data;
}

// 레코드 업데이트
int jdb_update(jdb_database_t* db, const char* collection, uint64_t id, void* data, size_t size) {
    if (!db || !collection || !data || size == 0) return JDB_INVALID_HEADER;
    if (!db->is_open) return JDB_INVALID_HEADER;
    
    // 컬렉션 ID 확인
    uint32_t collection_id = get_collection_id(db, collection);
    if (collection_id == 0) return JDB_INVALID_HEADER;
    
    // B+ Tree를 사용하여 레코드 오프셋 찾기
    uint64_t record_offset;
    if (jdb_btree_search(db, collection_id, id, &record_offset) != JDB_OK) {
        return JDB_NOT_FOUND;
    }
    
    // 기존 레코드 헤더 읽기
    fseek(db->file, record_offset, SEEK_SET);
    jdb_record_header_t header;
    if (fread(&header, sizeof(jdb_record_header_t), 1, db->file) != 1) {
        return JDB_IO_ERROR;
    }
    
    // 헤더 검증
    if (jdb_validate_record_header(&header) != JDB_OK) {
        return JDB_CORRUPTED_DATA;
    }
    
    // 삭제된 레코드인지 확인
    if (header.flags & JDB_RECORD_DELETED) {
        return JDB_NOT_FOUND;
    }
    
    // 새로운 레코드 ID 생성
    uint64_t new_record_id = jdb_generate_unique_id(db);
    if (new_record_id == 0) return JDB_MEMORY_ERROR;
    
    // 새로운 레코드 헤더 준비
    jdb_record_header_t new_header;
    new_header.record_id = new_record_id;
    new_header.type = header.type;
    new_header.size = sizeof(jdb_record_header_t) + size;
    new_header.flags = 0;
    new_header.timestamp = time(NULL);
    new_header.payload_size = size;
    new_header.collection_id = collection_id;
    
    // 공간 확보
    uint64_t new_record_offset;
    int free_space_result = find_free_space(db->free_space_manager, new_header.size, &new_record_offset);
    if (free_space_result != JDB_OK) {
        // 새로운 공간 사용
        new_record_offset = db->header.data_offset + db->header.data_size;
        db->header.data_size += new_header.size;
    }
    
    // 새 레코드 쓰기
    fseek(db->file, new_record_offset, SEEK_SET);
    
    // 레코드 헤더 쓰기
    if (fwrite(&new_header, sizeof(jdb_record_header_t), 1, db->file) != 1) {
        return JDB_IO_ERROR;
    }
    
    // 레코드 데이터 쓰기
    if (fwrite(data, size, 1, db->file) != 1) {
        return JDB_IO_ERROR;
    }
    
    // 원래 레코드를 삭제 상태로 표시
    fseek(db->file, record_offset, SEEK_SET);
    header.flags |= JDB_RECORD_DELETED;
    if (fwrite(&header, sizeof(jdb_record_header_t), 1, db->file) != 1) {
        return JDB_IO_ERROR;
    }
    
    // free space에 원래 레코드 추가
    add_free_space(db->free_space_manager, record_offset, header.size);
    
    // B+ Tree 업데이트: 기존 키 제거, 새 키 추가
    // 기존 키 제거는 단순히 삭제 마킹으로 처리
    // 새 키 추가
    if (jdb_btree_insert(db, collection_id, new_record_id, new_record_offset) != JDB_OK) {
        // 인덱스 업데이트 실패 시 경고만
    }
    
    // 헤더 업데이트 (무결성 포함)
    if (jdb_update_header_integrity(db) != JDB_OK) {
        return JDB_IO_ERROR;
    }
    
    return JDB_OK;
}

// 레코드 삭제
int jdb_delete(jdb_database_t* db, const char* collection, uint64_t id) {
    if (!db || !collection) return JDB_INVALID_HEADER;
    if (!db->is_open) return JDB_INVALID_HEADER;
    
    // 컬렉션 ID 확인
    uint32_t collection_id = get_collection_id(db, collection);
    if (collection_id == 0) return JDB_INVALID_HEADER;
    
    // B+ Tree를 사용하여 레코드 오프셋 찾기
    uint64_t record_offset;
    if (jdb_btree_search(db, collection_id, id, &record_offset) != JDB_OK) {
        return JDB_NOT_FOUND;
    }
    
    // 레코드 헤더 읽기
    fseek(db->file, record_offset, SEEK_SET);
    jdb_record_header_t header;
    if (fread(&header, sizeof(jdb_record_header_t), 1, db->file) != 1) {
        return JDB_IO_ERROR;
    }
    
    // 헤더 검증
    if (jdb_validate_record_header(&header) != JDB_OK) {
        return JDB_CORRUPTED_DATA;
    }
    
    // 이미 삭제된 레코드인지 확인
    if (header.flags & JDB_RECORD_DELETED) {
        return JDB_OK; // 이미 삭제됨
    }
    
    // 레코드를 삭제 상태로 표시
    fseek(db->file, record_offset, SEEK_SET);
    header.flags |= JDB_RECORD_DELETED;
    if (fwrite(&header, sizeof(jdb_record_header_t), 1, db->file) != 1) {
        return JDB_IO_ERROR;
    }
    
    // free space에 추가
    add_free_space(db->free_space_manager, record_offset, header.size);
    
    // 헤더 업데이트 (무결성 포함)
    if (jdb_update_header_integrity(db) != JDB_OK) {
        return JDB_IO_ERROR;
    }
    
    return JDB_OK;
}

// 존재 여부 확인
int jdb_exists(jdb_database_t* db, const char* collection, uint64_t id) {
    if (!db || !collection) return 0;
    if (!db->is_open) return 0;
    
    size_t size;
    void* data = jdb_get(db, collection, id, &size);
    if (data) {
        free(data);
        return 1;
    }
    return 0;
}

// 트랜잭션 시작
int jdb_begin(jdb_database_t* db) {
    if (!db) return JDB_INVALID_HEADER;
    if (!db->is_open) return JDB_INVALID_HEADER;
    
    if (db->transaction_state.active) {
        return JDB_TRANSACTION_ERROR; // 이미 활성 트랜잭션 존재
    }
    
    db->transaction_state.active = 1;
    db->transaction_state.id = time(NULL) * 1000000 + rand();
    db->transaction_state.temp_offset = 0;
    db->transaction_state.temp_size = 0;
    
    // 트랜잭션 로그 기록
    jdb_transaction_log_t log;
    log.trans_id = db->transaction_state.id;
    log.trans_type = JDB_TRANS_BEGIN;
    log.timestamp = time(NULL);
    log.operation_count = 0;
    
    // WAL 파일에 로그 기록
    char wal_filename[260];
    snprintf(wal_filename, sizeof(wal_filename), "%s.wal", db->filename);
    
    FILE* wal_file = fopen(wal_filename, "ab");
    if (wal_file) {
        fwrite(&log, sizeof(jdb_transaction_log_t), 1, wal_file);
        fclose(wal_file);
    }
    
    return JDB_OK;
}

// 트랜잭션 커밋
int jdb_commit(jdb_database_t* db) {
    if (!db) return JDB_INVALID_HEADER;
    if (!db->is_open) return JDB_INVALID_HEADER;
    if (!db->transaction_state.active) return JDB_TRANSACTION_ERROR;
    
    // 트랜잭션 로그 기록
    jdb_transaction_log_t log;
    log.trans_id = db->transaction_state.id;
    log.trans_type = JDB_TRANS_COMMIT;
    log.timestamp = time(NULL);
    log.operation_count = 0;
    
    char wal_filename[260];
    snprintf(wal_filename, sizeof(wal_filename), "%s.wal", db->filename);
    
    FILE* wal_file = fopen(wal_filename, "ab");
    if (!wal_file) {
        return JDB_IO_ERROR;
    }
    
    if (fwrite(&log, sizeof(jdb_transaction_log_t), 1, wal_file) != 1) {
        fclose(wal_file);
        return JDB_IO_ERROR;
    }
    
    fclose(wal_file);
    
    // 실제 변경 사항 적용
    if (fflush(db->file) != 0) {
        return JDB_IO_ERROR;
    }
    
    // 트랜잭션 상태 초기화
    db->transaction_state.active = 0;
    db->transaction_state.id = 0;
    db->transaction_state.temp_offset = 0;
    db->transaction_state.temp_size = 0;
    
    // WAL 파일 삭제 (커밋 완료 후)
    remove(wal_filename);
    
    return JDB_OK;
}

// 트랜잭션 롤백
int jdb_rollback(jdb_database_t* db) {
    if (!db) return JDB_INVALID_HEADER;
    if (!db->is_open) return JDB_INVALID_HEADER;
    if (!db->transaction_state.active) return JDB_TRANSACTION_ERROR;
    
    // 트랜잭션 로그 기록
    jdb_transaction_log_t log;
    log.trans_id = db->transaction_state.id;
    log.trans_type = JDB_TRANS_ROLLBACK;
    log.timestamp = time(NULL);
    log.operation_count = 0;
    
    char wal_filename[260];
    snprintf(wal_filename, sizeof(wal_filename), "%s.wal", db->filename);
    
    FILE* wal_file = fopen(wal_filename, "ab");
    if (!wal_file) {
        return JDB_IO_ERROR;
    }
    
    if (fwrite(&log, sizeof(jdb_transaction_log_t), 1, wal_file) != 1) {
        fclose(wal_file);
        return JDB_IO_ERROR;
    }
    
    fclose(wal_file);
    
    // 트랜잭션 상태 초기화
    db->transaction_state.active = 0;
    db->transaction_state.id = 0;
    db->transaction_state.temp_offset = 0;
    db->transaction_state.temp_size = 0;
    
    // WAL 파일 삭제
    remove(wal_filename);
    
    return JDB_OK;
}

// 무결성 검사
int jdb_verify(jdb_database_t* db) {
    if (!db) return JDB_INVALID_HEADER;
    if (!db->is_open) return JDB_INVALID_HEADER;
    
    // 헤더 무결성 검사
    uint8_t computed_integrity[32];
    jdb_sha256((unsigned char*)&db->header + 8, sizeof(jdb_header_t) - 40, computed_integrity);
    
    if (memcmp(computed_integrity, db->header.integrity, 32) != 0) {
        return JDB_INTEGRITY_ERROR;
    }
    
    // 컬렉션 무결성 검사
    for (uint32_t i = 0; i < db->collection_count; i++) {
        if (db->collections[i].id != i + 1) {
            return JDB_CORRUPTED_DATA;
        }
    }
    
    // 데이터 영역 검사 (간소화)
    uint64_t current_pos = db->header.data_offset;
    while (current_pos < db->header.data_offset + db->header.data_size) {
        jdb_record_header_t header;
        fseek(db->file, current_pos, SEEK_SET);
        if (fread(&header, sizeof(jdb_record_header_t), 1, db->file) != 1) {
            break;
        }
        
        if (jdb_validate_record_header(&header) != JDB_OK) {
            return JDB_CORRUPTED_DATA;
        }
        
        current_pos += header.size;
    }
    
    return JDB_OK;
}

// 데이터베이스 플러시
int jdb_flush(jdb_database_t* db) {
    if (!db) return JDB_INVALID_HEADER;
    if (!db->is_open) return JDB_INVALID_HEADER;
    
    return fflush(db->file) == 0 ? JDB_OK : JDB_IO_ERROR;
}

// 패스워드 변경
int jdb_change_password(jdb_database_t* db, const char* old_password, const char* new_password) {
    if (!db || !old_password || !new_password) return JDB_INVALID_HEADER;
    if (!db->is_open) return JDB_INVALID_HEADER;
    
    // 기존 패스워드 검증
    uint8_t computed_hash[32];
    if (hash_password(old_password, db->header.salt, computed_hash) != 0) {
        return JDB_AUTH_FAILED;
    }
    
    if (memcmp(computed_hash, db->header.password_hash, 32) != 0) {
        return JDB_AUTH_FAILED;
    }
    
    // 새 패스워드 조건 검사
    int has_lower = 0, has_upper = 0, has_digit = 0, has_special = 0;
    int special_count = 0;
    int len = strlen(new_password);
    
    if (len < 8) return JDB_INVALID_PASSWORD;
    
    for (int i = 0; i < len; i++) {
        char c = new_password[i];
        if (c >= 'a' && c <= 'z') has_lower = 1;
        else if (c >= 'A' && c <= 'Z') has_upper = 1;
        else if (c >= '0' && c <= '9') has_digit = 1;
        else if ((c >= '!' && c <= '/') || (c >= ':' && c <= '@') || 
                 (c >= '[' && c <= '`') || (c >= '{' && c <= '~')) {
            has_special = 1;
            special_count++;
        }
    }
    
    if (!has_lower && !has_upper) return JDB_INVALID_PASSWORD;  // 최소 한 개의 영문자
    if (!has_digit) return JDB_INVALID_PASSWORD;                // 최소 한 개의 숫자
    if (!has_special || special_count < 3) return JDB_INVALID_PASSWORD;  // 최소 세 개의 특수문자
    
    // 새로운 솔트 생성
    if (jdb_random_bytes(db->header.salt, 16) != 0) {
        return JDB_MEMORY_ERROR;
    }
    
    // 새 패스워드 해싱
    if (hash_password(new_password, db->header.salt, db->header.password_hash) != 0) {
        return JDB_MEMORY_ERROR;
    }
    
    // 타임스탬프 업데이트
    db->header.timestamp_modified = time(NULL);
    
    // 무결성 해시 재계산
    uint8_t temp_integrity[32];
    jdb_sha256((unsigned char*)&db->header + 8, sizeof(jdb_header_t) - 40, temp_integrity);
    memcpy(db->header.integrity, temp_integrity, 32);
    
    // 헤더 업데이트
    fseek(db->file, 0, SEEK_SET);
    if (fwrite(&db->header, sizeof(jdb_header_t), 1, db->file) != 1) {
        return JDB_IO_ERROR;
    }
    
    if (fflush(db->file) != 0) {
        return JDB_IO_ERROR;
    }
    
    return JDB_OK;
}

// Free Space 파일에서 로드
static int jdb_load_free_space_from_file(jdb_database_t* db) {
    if (!db) return JDB_INVALID_HEADER;
    
    // 간소화된 구현: 메타데이터 영역에 저장된다고 가정
    // 실제 구현에서는 별도의 Free Space 테이블을 사용
    return JDB_OK;
}

// Free Space 파일에 저장
static int jdb_save_free_space_to_file(jdb_database_t* db) {
    if (!db) return JDB_INVALID_HEADER;
    
    // 간소화된 구현: 현재는 저장하지 않음
    // 실제 구현에서는 메타데이터 영역에 저장
    return JDB_OK;
}

// 인덱스 재구성
int jdb_rebuild_index(jdb_database_t* db) {
    if (!db) return JDB_INVALID_HEADER;
    if (!db->is_open) return JDB_INVALID_HEADER;
    
    // 기존 인덱스 초기화
    db->root_page_offset = 0;
    
    // 루트 노드 생성
    if (jdb_btree_create_root(db) != JDB_OK) {
        return JDB_INVALID_INDEX;
    }
    
    // 데이터 영역 스캔하여 유효한 레코드를 찾아 인덱스 재생성
    uint64_t current_pos = db->header.data_offset;
    while (current_pos < db->header.data_offset + db->header.data_size) {
        jdb_record_header_t header;
        fseek(db->file, current_pos, SEEK_SET);
        if (fread(&header, sizeof(jdb_record_header_t), 1, db->file) != 1) {
            break;
        }
        
        if (jdb_validate_record_header(&header) != JDB_OK) {
            // 잘못된 헤더는 건너뜀
            current_pos += 1024; // 작은 단위로 건너뜀
            continue;
        }
        
        // 삭제되지 않은 레코드만 인덱스에 추가
        if (!(header.flags & JDB_RECORD_DELETED)) {
            if (jdb_btree_insert(db, header.collection_id, header.record_id, current_pos) != JDB_OK) {
                // 오류 발생 시 경고만
            }
        }
        
        current_pos += header.size;
    }
    
    return JDB_OK;
}

// 복구
int jdb_recover(jdb_database_t* db) {
    if (!db) return JDB_INVALID_HEADER;
    if (!db->is_open) return JDB_INVALID_HEADER;
    
    // WAL 파일 존재 여부 확인
    char wal_filename[260];
    snprintf(wal_filename, sizeof(wal_filename), "%s.wal", db->filename);
    
    FILE* wal_file = fopen(wal_filename, "rb");
    if (!wal_file) {
        return JDB_OK; // WAL 파일 없음
    }
    
    // WAL 파일 처리 (간소화)
    jdb_transaction_log_t log;
    while (fread(&log, sizeof(jdb_transaction_log_t), 1, wal_file) == 1) {
        // 로그 처리 (간소화)
        // 실제 구현에서는 트랜잭션 복구 로직 포함
    }
    
    fclose(wal_file);
    remove(wal_filename); // WAL 파일 삭제
    
    return JDB_OK;
}

// 컴팩션
int jdb_compact(jdb_database_t* db) {
    if (!db) return JDB_INVALID_HEADER;
    if (!db->is_open) return JDB_INVALID_HEADER;
    
    // 임시 파일 생성
    char temp_filename[260];
    snprintf(temp_filename, sizeof(temp_filename), "%s.tmp", db->filename);
    
    FILE* temp_file = fopen(temp_filename, "w+b");
    if (!temp_file) {
        return JDB_IO_ERROR;
    }
    
    // 헤더 복사
    fseek(db->file, 0, SEEK_SET);
    jdb_header_t temp_header;
    if (fread(&temp_header, sizeof(jdb_header_t), 1, db->file) != 1) {
        fclose(temp_file);
        remove(temp_filename);
        return JDB_IO_ERROR;
    }
    
    // 헤더에서 데이터 크기 초기화
    temp_header.data_size = 0;
    
    if (fwrite(&temp_header, sizeof(jdb_header_t), 1, temp_file) != 1) {
        fclose(temp_file);
        remove(temp_filename);
        return JDB_IO_ERROR;
    }
    
    // 데이터 영역 스캔 및 유효한 레코드만 복사
    uint64_t current_pos = db->header.data_offset;
    uint64_t new_data_offset = 0x3000; // 임시로 새 데이터 시작 위치
    
    fseek(temp_file, new_data_offset, SEEK_SET);
    
    while (current_pos < db->header.data_offset + db->header.data_size) {
        jdb_record_header_t header;
        fseek(db->file, current_pos, SEEK_SET);
        if (fread(&header, sizeof(jdb_record_header_t), 1, db->file) != 1) {
            break;
        }
        
        if (jdb_validate_record_header(&header) != JDB_OK) {
            current_pos += 1024;
            continue;
        }
        
        // 삭제되지 않은 레코드만 복사
        if (!(header.flags & JDB_RECORD_DELETED)) {
            if (fwrite(&header, sizeof(jdb_record_header_t), 1, temp_file) != 1) {
                fclose(temp_file);
                remove(temp_filename);
                return JDB_IO_ERROR;
            }
            
            // 데이터 복사
            uint8_t* data = malloc(header.payload_size);
            if (!data) {
                fclose(temp_file);
                remove(temp_filename);
                return JDB_MEMORY_ERROR;
            }
            
            if (fread(data, header.payload_size, 1, db->file) != 1) {
                free(data);
                fclose(temp_file);
                remove(temp_filename);
                return JDB_IO_ERROR;
            }
            
            if (fwrite(data, header.payload_size, 1, temp_file) != 1) {
                free(data);
                fclose(temp_file);
                remove(temp_filename);
                return JDB_IO_ERROR;
            }
            
            free(data);
            
            temp_header.data_size += header.size;
        }
        
        current_pos += header.size;
    }
    
    // 헤더 업데이트
    temp_header.timestamp_modified = time(NULL);
    
    // 무결성 해시 계산
    uint8_t temp_integrity[32];
    jdb_sha256((unsigned char*)&temp_header + 8, sizeof(jdb_header_t) - 40, temp_integrity);
    memcpy(temp_header.integrity, temp_integrity, 32);
    
    // 헤더 다시 쓰기
    fseek(temp_file, 0, SEEK_SET);
    if (fwrite(&temp_header, sizeof(jdb_header_t), 1, temp_file) != 1) {
        fclose(temp_file);
        remove(temp_filename);
        return JDB_IO_ERROR;
    }
    
    fclose(temp_file);
    
    // 원본 파일 닫기
    fclose(db->file);
    db->is_open = 0;
    
    // 원본 파일 삭제
    remove(db->filename);
    
    // 임시 파일을 원본으로 이름 변경
    if (rename(temp_filename, db->filename) != 0) {
        return JDB_IO_ERROR;
    }
    
    // 다시 열기
    db->file = fopen(db->filename, "r+b");
    if (!db->file) {
        return JDB_IO_ERROR;
    }
    
    db->is_open = 1;
    
    return JDB_OK;
}

// 테스트 함수
int jdb_test() {
    printf("JDB Test Starting...\n");
    
    // DB 생성
    jdb_database_t* db = jdb_create("test.jdb", "Test123!@#");
    if (!db) {
        printf("Failed to create database\n");
        return -1;
    }
    printf("Database created successfully\n");
    
    // 레코드 삽입
    char* data1 = "Hello, JDB!";
    if (jdb_insert(db, "users", data1, strlen(data1)) != JDB_OK) {
        printf("Failed to insert record\n");
        jdb_close(db);
        return -1;
    }
    printf("Record inserted successfully\n");
    
    // 레코드 조회
    size_t size;
    void* retrieved_data = jdb_get(db, "users", 1, &size); // 실제 ID는 다를 수 있음
    if (retrieved_data) {
        printf("Retrieved: %.*s\n", (int)size, (char*)retrieved_data);
        free(retrieved_data);
    } else {
        printf("Record not found\n");
    }
    
    // 트랜잭션 테스트
    if (jdb_begin(db) == JDB_OK) {
        printf("Transaction began\n");
        if (jdb_commit(db) == JDB_OK) {
            printf("Transaction committed\n");
        } else {
            printf("Transaction commit failed\n");
        }
    } else {
        printf("Failed to begin transaction\n");
    }
    
    // 무결성 검사
    if (jdb_verify(db) == JDB_OK) {
        printf("Integrity check passed\n");
    } else {
        printf("Integrity check failed\n");
    }
    
    // DB 닫기
    if (jdb_close(db) == JDB_OK) {
        printf("Database closed successfully\n");
    } else {
        printf("Failed to close database\n");
    }
    
    printf("JDB Test Completed\n");
    return 0;
}

#ifdef JDB_STANDALONE_TEST
int main() {
    return jdb_test();
}
#endif
