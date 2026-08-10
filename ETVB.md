
# 엄청난걸 만들었어요! ETVB라는걸 만들었습네다. 
> **Extract the virtual brain에 약자를 따서 ETVB가 됐습네다.**
### 별로 실용성 있진 않아요 ㅠ






```/**
 * ============================================================================
 * Etvb (Extract the Virtual Brain) v1.0 - Reference Implementation
 * ============================================================================
 * 
 * [규격서] Etvb Binary Format Specification v1.0
 * ------------------------------------------------
 * 
 * 1. 개요
 *    Etvb는 AI 모델의 가중치(Neural Core), 지식 위상(Knowledge Topology), 
 *    행동 성향(Behavioral Profile), 추론 전략(Cognitive Schema)을 단일 바이너리 
 *    파일(.etvb)로 압축하여 저장하는 포맷입니다.
 * 
 * 2. 파일 레이아웃 (Little-Endian)
 *    +---------------------+ Offset 0
 *    | Header (128 Bytes)  |
 *    +---------------------+ Offset 128
 *    | Section Index Table | (Entry Count * 48 Bytes)
 *    +---------------------+
 *    | Padding             | (To align next section to 64 bytes)
 *    +---------------------+
 *    | Section Data Blocks | (Each aligned to 64 bytes)
 *    | ...                 |
 *    +---------------------+
 *    | Footer (32 Bytes)   | (XXH3-128 Hash of all preceding data)
 *    +---------------------+
 * 
 * 3. 핵심 구조체 (Packed, No Padding)
 * 
 *    A. etvb_header_t (128 Bytes)
 *       - magic: uint32_t (0x42565445 "ETVB")
 *       - ver_major/minor: uint16_t
 *       - package_id: uint8_t[16] (UUID)
 *       - name_len/offset: uint32_t
 *       - base_model_hash: uint8_t[32] (SHA-256)
 *       - architecture: uint32_t (Enum)
 *       - param_count_B: uint32_t
 *       - min_runtime_ver: uint32_t
 *       - required_caps: uint32_t (Bitfield)
 *       - estimated_vram_mb: uint64_t
 *       - compression_algo: uint32_t (0=None, 1=Zstd, 2=Lz4)
 *       - encryption_scheme: uint32_t
 *       - section_count: uint32_t
 *       - index_offset: uint64_t
 *       - reserved: uint8_t[4]
 * 
 *    B. etvb_section_entry_t (48 Bytes)
 *       - type: uint32_t (Section Type Enum)
 *       - version: uint16_t
 *       - alignment_exp: uint8_t (2^N bytes, usually 6 for 64B)
 *       - flags: uint8_t (bit0: compressed)
 *       - data_offset: uint64_t (Absolute file offset)
 *       - data_size: uint64_t (Compressed size on disk)
 *       - uncompressed_size: uint64_t (Original size in memory)
 *       - checksum: uint32_t (CRC32C of raw data)
 *       - reserved: uint32_t
 * 
 *    C. etvb_footer_t (32 Bytes)
 *       - xxh3_hash: uint8_t[16] (XXH3-128 of Header + Index + All Data)
 *       - reserved: uint8_t[16]
 * 
 * 4. 정렬 규칙 (Alignment Rules)
 *    - 모든 섹션 데이터 블록은 64바이트 경계에 정렬되어야 함.
 *    - 인덱스 테이블은 헤더 직후에 위치하며 정렬되지 않을 수 있음.
 *    - 문자열 풀(String Pool)은 별도의 섹션 또는 데이터 블록 내에 포함.
 * 
 * 5. 무결성 검증 (Integrity)
 *    - 각 섹션은 CRC32C 체크섬을 가짐.
 *    - 전체 파일(푸터 제외)은 XXH3-128 해시로 검증됨.
 * 
 * ============================================================================
 * 컴파일 방법: gcc -O2 -o etvb etvb.c -lm
 * 사용법:
 *   ./etvb pack <model_dir> <output.etvb> [name] [arch_id]
 *   ./etvb unpack <input.etvb> <output_dir>
 *   ./etvb validate <input.etvb>
 * ============================================================================
 */

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <stdint.h>
#include <stdbool.h>
#include <errno.h>
#include <math.h>
#include <time.h>

/* ==========================================================================
 * 1. 상수 및 타입 정의 (Constants & Types)
 * ========================================================================== */

#define ETVB_MAGIC          0x42565445U // "ETVB" in Little-Endian
#define ETVB_VERSION_MAJOR  1
#define ETVB_VERSION_MINOR  0
#define ETVB_ALIGNMENT      64
#define ETVB_MAX_SECTIONS   64
#define ETVB_BUFFER_SIZE    (1024 * 1024) // 1MB Read Buffer

/* 에러 코드 */
typedef enum {
    ETVB_OK = 0,
    ETVB_ERR_NULL_PTR,
    ETVB_ERR_IO,
    ETVB_ERR_MAGIC,
    ETVB_ERR_VERSION,
    ETVB_ERR_ALIGNMENT,
    ETVB_ERR_BOUNDS,
    ETVB_ERR_CHECKSUM,
    ETVB_ERR_NOT_FOUND,
    ETVB_ERR_INVALID_STATE,
    ETVB_ERR_MEMORY
} etvb_error_t;

/* 아키텍처 ID */
typedef enum {
    ETVB_ARCH_UNKNOWN = 0,
    ETVB_ARCH_GEMMA_2B = 1,
    ETVB_ARCH_GEMMA_9B = 2,
    ETVB_ARCH_LLAMA_7B = 3,
    ETVB_ARCH_CUSTOM   = 0xFFFF
} etvb_architecture_t;

/* 압축 타입 */
typedef enum {
    ETVB_COMP_NONE = 0,
    ETVB_COMP_ZSTD = 1,
    ETVB_COMP_LZ4  = 2
} etvb_compression_t;

/* 양자화 타입 */
typedef enum {
    ETVB_QUANT_FP32 = 0,
    ETVB_QUANT_FP16 = 1,
    ETVB_QUANT_INT8 = 3,
    ETVB_QUANT_INT4 = 4
} etvb_quantization_t;

/* 섹션 타입 */
typedef enum {
    ETVB_SEC_NEURAL_CORE       = 0x0001,
    ETVB_SEC_KNOWLEDGE_TOPO    = 0x0002,
    ETVB_SEC_BEHAVIORAL        = 0x0003,
    ETVB_SEC_COGNITIVE         = 0x0004,
    ETVB_SEC_BINDING           = 0x0005,
    ETVB_SEC_METADATA          = 0x00FF
} etvb_section_type_t;

/* ==========================================================================
 * 2. 구조체 정의 (Structures) - Packed
 * ========================================================================== */

#pragma pack(push, 1)

typedef struct {
    uint32_t magic;
    uint16_t ver_major;
    uint16_t ver_minor;
    uint8_t  package_id[16];
    uint32_t name_len;
    uint32_t name_offset;
    uint8_t  base_model_hash[32];
    uint32_t architecture;
    uint32_t param_count_B;
    uint32_t min_runtime_ver;
    uint32_t required_caps;
    uint64_t estimated_vram_mb;
    uint32_t compression_algo;
    uint32_t encryption_scheme;
    uint32_t section_count;
    uint64_t index_offset;
    uint8_t  reserved[4];
} etvb_header_t; // 128 bytes

typedef struct {
    uint32_t type;
    uint16_t version;
    uint8_t  alignment_exp;
    uint8_t  flags;
    uint64_t data_offset;
    uint64_t data_size;
    uint64_t uncompressed_size;
    uint32_t checksum;
    uint32_t reserved;
} etvb_section_entry_t; // 48 bytes

typedef struct {
    uint16_t verbosity;
    uint16_t formality;
    uint16_t creativity;
    int16_t  confidence_bias;
    uint32_t safety_level;
    uint32_t refusal_policy;
    uint32_t persona_name_len;
    uint32_t persona_name_offset;
    uint32_t system_prompt_len;
    uint32_t system_prompt_offset;
} etvb_behavioral_t;

typedef struct {
    uint8_t  xxh3_hash[16];
    uint8_t  reserved[16];
} etvb_footer_t;

#pragma pack(pop)

/* ==========================================================================
 * 3. 유틸리티 함수 (Utilities)
 * ========================================================================== */

static inline uint64_t align_up(uint64_t offset, uint64_t align) {
    return (offset + align - 1) & ~(align - 1);
}

static const char* etvb_strerror(etvb_error_t err) {
    switch(err) {
        case ETVB_OK: return "Success";
        case ETVB_ERR_NULL_PTR: return "Null pointer error";
        case ETVB_ERR_IO: return "I/O error";
        case ETVB_ERR_MAGIC: return "Invalid magic number";
        case ETVB_ERR_VERSION: return "Unsupported version";
        case ETVB_ERR_ALIGNMENT: return "Alignment error";
        case ETVB_ERR_BOUNDS: return "Out of bounds access";
        case ETVB_ERR_CHECKSUM: return "Checksum verification failed";
        case ETVB_ERR_NOT_FOUND: return "Section not found";
        case ETVB_ERR_MEMORY: return "Memory allocation failed";
        default: return "Unknown error";
    }
}

/* --------------------------------------------------------------------------
 * XXH3-128 Simplified Implementation (For Single File Portability)
 * 실제 프로덕션에서는 xxhash 라이브러리 사용을 강력 권장합니다.
 * -------------------------------------------------------------------------- */
typedef struct {
    uint64_t acc[8];
    uint8_t  buffer[256];
    size_t   bufferedSize;
    uint64_t totalLen;
} xxh3_state_t;

static void xxh3_reset(xxh3_state_t* state) {
    memset(state, 0, sizeof(xxh3_state_t));
    // Initial seeds (simplified)
    for(int i=0; i<8; i++) state->acc[i] = (uint64_t)i * 0x9E3779B97F4A7C15ULL;
}

static void xxh3_update(xxh3_state_t* state, const void* input, size_t length) {
    const uint8_t* p = (const uint8_t*)input;
    state->totalLen += length;
    
    // Simple accumulation for demo purposes (Not cryptographically secure or fast)
    // In a real scenario, use proper XXH3 blocks.
    for(size_t i=0; i<length; i++) {
        state->acc[i % 8] ^= ((uint64_t)p[i] << (i % 56));
        state->acc[i % 8] *= 0x9E3779B97F4A7C15ULL;
    }
}

static void xxh3_digest(const xxh3_state_t* state, uint8_t out[16]) {
    // Mix accumulators into output
    uint64_t h1 = state->acc[0] ^ state->acc[1];
    uint64_t h2 = state->acc[2] ^ state->acc[3];
    memcpy(out, &h1, 8);
    memcpy(out + 8, &h2, 8);
}

/* CRC32C Software Implementation */
static uint32_t crc32c_sw(const uint8_t* data, size_t length) {
    uint32_t crc = 0xFFFFFFFF;
    // Standard CRC32C polynomial: 0x1EDC6F41 (Castagnoli)
    // Using a simple byte-by-byte calculation for portability without tables
    for (size_t i = 0; i < length; i++) {
        crc ^= data[i];
        for (int j = 0; j < 8; j++) {
            if (crc & 1)
                crc = (crc >> 1) ^ 0x82F63B78;
            else
                crc >>= 1;
        }
    }
    return crc ^ 0xFFFFFFFF;
}

/* ==========================================================================
 * 4. 핵심 로직: Pack (Model -> Etvb)
 * ========================================================================== */

etvb_error_t etvb_pack_internal(FILE* fp_out, const char* model_path, const char* name, 
                                etvb_architecture_t arch, etvb_compression_t comp) {
    etvb_error_t err = ETVB_OK;
    size_t total_written = 0;
    
    // 1. 헤더 초기화
    etvb_header_t header = {0};
    header.magic = ETVB_MAGIC;
    header.ver_major = ETVB_VERSION_MAJOR;
    header.ver_minor = ETVB_VERSION_MINOR;
    header.architecture = arch;
    header.compression_algo = comp;
    
    // UUID 생성 (간소화: 시간 기반)
    uint64_t t = (uint64_t)time(NULL);
    memcpy(header.package_id, &t, 8);
    memset(header.package_id + 8, 0xAB, 8); // Dummy padding

    // 패키지 이름 처리
    if (name) {
        header.name_len = (uint32_t)strlen(name);
    }

    // 헤더 쓰기 (임시, 나중에 업데이트)
    if (fwrite(&header, sizeof(header), 1, fp_out) != 1) return ETVB_ERR_IO;
    total_written += sizeof(header);

    // 2. 섹션 인덱스 준비
    // 데모를 위해 NEURAL_CORE와 BEHAVIORAL 두 개의 섹션을 생성합니다.
    etvb_section_entry_t entries[2] = {0};
    uint32_t sec_count = 0;

    // --- 섹션 1: NEURAL_CORE (더미 가중치) ---
    const char* dummy_weights = "DUMMY_WEIGHT_DATA_BLOCK_FOR_NEURAL_CORE";
    size_t w_size = strlen(dummy_weights);
    
    entries[0].type = ETVB_SEC_NEURAL_CORE;
    entries[0].version = 1;
    entries[0].alignment_exp = 6; // 64 bytes
    entries[0].flags = 0;
    entries[0].uncompressed_size = w_size;
    entries[0].checksum = crc32c_sw((const uint8_t*)dummy_weights, w_size);
    sec_count++;

    // --- 섹션 2: BEHAVIORAL (더미 프로필) ---
    etvb_behavioral_t behavior = {0};
    behavior.verbosity = 32768; // 0.5
    behavior.formality = 65535; // 1.0
    behavior.safety_level = 2;
    
    entries[1].type = ETVB_SEC_BEHAVIORAL;
    entries[1].version = 1;
    entries[1].alignment_exp = 6;
    entries[1].flags = 0;
    entries[1].uncompressed_size = sizeof(etvb_behavioral_t);
    entries[1].checksum = crc32c_sw((const uint8_t*)&behavior, sizeof(behavior));
    sec_count++;

    header.section_count = sec_count;

    // 3. 인덱스 테이블 위치 계산 및 쓰기
    header.index_offset = total_written;
    size_t index_size = sizeof(etvb_section_entry_t) * header.section_count;
    
    // 인덱스 쓰기 (데이터 오프셋은 아직 0)
    if (fwrite(entries, sizeof(etvb_section_entry_t), header.section_count, fp_out) != header.section_count) {
        return ETVB_ERR_IO;
    }
    total_written += index_size;

    // 4. 섹션 데이터 쓰기 (64바이트 정렬 준수)
    for (uint32_t i = 0; i < header.section_count; i++) {
        uint64_t aligned_off = align_up(total_written, ETVB_ALIGNMENT);
        
        // 패딩 쓰기
        uint8_t pad[ETVB_ALIGNMENT] = {0};
        size_t pad_size = aligned_off - total_written;
        if (pad_size > 0) {
            if (fwrite(pad, 1, pad_size, fp_out) != pad_size) return ETVB_ERR_IO;
            total_written += pad_size;
        }
        
        entries[i].data_offset = aligned_off;
        
        // 실제 데이터 쓰기
        if (entries[i].type == ETVB_SEC_NEURAL_CORE) {
            if (fwrite(dummy_weights, 1, w_size, fp_out) != w_size) return ETVB_ERR_IO;
            entries[i].data_size = w_size;
            total_written += w_size;
        } else if (entries[i].type == ETVB_SEC_BEHAVIORAL) {
            if (fwrite(&behavior, 1, sizeof(behavior), fp_out) != sizeof(behavior)) return ETVB_ERR_IO;
            entries[i].data_size = sizeof(behavior);
            total_written += sizeof(behavior);
        }
    }

    // 5. 문자열 풀 (Name) 쓰기 (선택적)
    if (name && header.name_len > 0) {
        uint64_t str_off = align_up(total_written, ETVB_ALIGNMENT);
        uint8_t pad2[ETVB_ALIGNMENT] = {0};
        size_t pad2_size = str_off - total_written;
        if (pad2_size > 0) {
            fwrite(pad2, 1, pad2_size, fp_out);
            total_written += pad2_size;
        }
        
        // 헤더의 name_offset은 파일 시작 기준 절대 오프셋으로 저장
        header.name_offset = (uint32_t)str_off;
        
        if (fwrite(name, 1, header.name_len, fp_out) != header.name_len) return ETVB_ERR_IO;
        total_written += header.name_len;
    }

    // 6. 푸터 쓰기 전 해시 계산
    // 파일 포인터를 처음으로 되돌려 전체 데이터를 읽어 해시를 계산합니다.
    // 대용량 파일에서는 스트리밍 해시를 사용하는 것이 좋으나, 여기서는 단순화를 위해 전체 읽기.
    fseek(fp_out, 0, SEEK_SET);
    
    xxh3_state_t hash_state;
    xxh3_reset(&hash_state);
    
    uint8_t* read_buf = malloc(ETVB_BUFFER_SIZE);
    if (!read_buf) return ETVB_ERR_MEMORY;
    
    size_t bytes_to_hash = total_written; // 푸터 제외 크기
    size_t remaining = bytes_to_hash;
    
    while (remaining > 0) {
        size_t to_read = (remaining > ETVB_BUFFER_SIZE) ? ETVB_BUFFER_SIZE : remaining;
        size_t read_count = fread(read_buf, 1, to_read, fp_out);
        if (read_count != to_read) {
            free(read_buf);
            return ETVB_ERR_IO;
        }
        xxh3_update(&hash_state, read_buf, read_count);
        remaining -= read_count;
    }
    free(read_buf);
    
    etvb_footer_t footer = {0};
    xxh3_digest(&hash_state, footer.xxh3_hash);
    
    // 푸터 쓰기
    if (fwrite(&footer, sizeof(footer), 1, fp_out) != 1) return ETVB_ERR_IO;
    total_written += sizeof(footer);

    // 7. 헤더 및 인덱스 업데이트 (Seek Back)
    fseek(fp_out, 0, SEEK_SET);
    fwrite(&header, sizeof(header), 1, fp_out);
    
    fseek(fp_out, header.index_offset, SEEK_SET);
    fwrite(entries, sizeof(etvb_section_entry_t), header.section_count, fp_out);

    return ETVB_OK;
}

etvb_error_t etvb_pack(const char* model_path, const char* output_path, const char* name,
                       etvb_architecture_t arch, etvb_compression_t comp, etvb_quantization_t quant) {
    if (!model_path || !output_path) return ETVB_ERR_NULL_PTR;

    FILE* fp = fopen(output_path, "wb");
    if (!fp) return ETVB_ERR_IO;

    etvb_error_t err = etvb_pack_internal(fp, model_path, name, arch, comp);
    
    fclose(fp);
    return err;
}

/* ==========================================================================
 * 5. 핵심 로직: Unpack (Etvb -> Model)
 * ========================================================================== */

etvb_error_t etvb_unpack(const char* etvb_path, const char* output_dir, bool verify_only) {
    if (!etvb_path || !output_dir) return ETVB_ERR_NULL_PTR;

    FILE* fp = fopen(etvb_path, "rb");
    if (!fp) return ETVB_ERR_IO;

    // 1. 헤더 읽기 및 검증
    etvb_header_t header;
    if (fread(&header, sizeof(header), 1, fp) != 1) {
        fclose(fp);
        return ETVB_ERR_IO;
    }

    if (header.magic != ETVB_MAGIC) {
        fclose(fp);
        return ETVB_ERR_MAGIC;
    }
    if (header.ver_major != ETVB_VERSION_MAJOR) {
        fclose(fp);
        return ETVB_ERR_VERSION;
    }

    // 2. 인덱스 테이블 읽기
    if (fseek(fp, header.index_offset, SEEK_SET) != 0) {
        fclose(fp);
        return ETVB_ERR_IO;
    }

    if (header.section_count > ETVB_MAX_SECTIONS) {
        fclose(fp);
        return ETVB_ERR_INVALID_STATE;
    }

    etvb_section_entry_t* entries = malloc(sizeof(etvb_section_entry_t) * header.section_count);
    if (!entries) {
        fclose(fp);
        return ETVB_ERR_MEMORY;
    }

    if (fread(entries, sizeof(etvb_section_entry_t), header.section_count, fp) != header.section_count) {
        free(entries);
        fclose(fp);
        return ETVB_ERR_IO;
    }

    // 3. 각 섹션 추출
    for (uint32_t i = 0; i < header.section_count; i++) {
        // Bounds Check: 데이터가 파일 크기를 넘지 않는지 확인 (간소화)
        // 실제로는 파일 끝(EOF)을 구해서 비교해야 함
        
        if (verify_only) {
            printf("[Verify] Section %d: Type=0x%X, Checksum=0x%08X\n", 
                   i, entries[i].type, entries[i].checksum);
            continue;
        }

        // 데이터 읽기
        uint8_t* buffer = malloc(entries[i].uncompressed_size);
        if (!buffer) {
            free(entries);
            fclose(fp);
            return ETVB_ERR_MEMORY;
        }

        if (fseek(fp, entries[i].data_offset, SEEK_SET) != 0) {
            free(buffer);
            free(entries);
            fclose(fp);
            return ETVB_ERR_IO;
        }

        if (fread(buffer, 1, entries[i].data_size, fp) != entries[i].data_size) {
            free(buffer);
            free(entries);
            fclose(fp);
            return ETVB_ERR_IO;
        }

        // 체크섬 검증
        uint32_t calc_crc = crc32c_sw(buffer, entries[i].data_size);
        if (calc_crc != entries[i].checksum) {
            fprintf(stderr, "Warning: Checksum mismatch in section %d!\n", i);
            // 에러로 처리할지 경고로 할지는 정책에 따름
        }

        // TODO: 압축 해제 (현재는 무압축 가정)
        
        // TODO: 파일로 저장 (섹션 타입에 따라 다른 확장자)
        printf("[Unpack] Extracted Section %d (Type: 0x%X). Size: %lu bytes.\n", 
               i, entries[i].type, entries[i].data_size);
        
        free(buffer);
    }

    free(entries);
    fclose(fp);
    return ETVB_OK;
}

/* ==========================================================================
 * 6. 핵심 로직: Validate
 * ========================================================================== */

etvb_error_t etvb_validate(const char* etvb_path) {
    FILE* fp = fopen(etvb_path, "rb");
    if (!fp) return ETVB_ERR_IO;

    // 파일 크기 구하기
    fseek(fp, 0, SEEK_END);
    long file_size = ftell(fp);
    fseek(fp, 0, SEEK_SET);

    if (file_size < (long)(sizeof(etvb_header_t) + sizeof(etvb_footer_t))) {
        fclose(fp);
        return ETVB_ERR_BOUNDS;
    }

    // 1. 헤더 검증
    etvb_header_t header;
    if (fread(&header, sizeof(header), 1, fp) != 1) {
        fclose(fp);
        return ETVB_ERR_IO;
    }

    if (header.magic != ETVB_MAGIC) {
        fclose(fp);
        return ETVB_ERR_MAGIC;
    }

    printf("[Validate] Magic: OK\n");
    printf("[Validate] Version: %d.%d\n", header.ver_major, header.ver_minor);
    printf("[Validate] Sections: %d\n", header.section_count);

    // 2. 인덱스 검증
    if (fseek(fp, header.index_offset, SEEK_SET) != 0) {
        fclose(fp);
        return ETVB_ERR_IO;
    }

    etvb_section_entry_t entry;
    for (uint32_t i = 0; i < header.section_count; i++) {
        if (fread(&entry, sizeof(entry), 1, fp) != 1) {
            fclose(fp);
            return ETVB_ERR_IO;
        }
        
        // 오프셋 유효성 검사
        if (entry.data_offset + entry.data_size > (uint64_t)file_size) {
            printf("[Validate] Error: Section %d exceeds file bounds.\n", i);
            fclose(fp);
            return ETVB_ERR_BOUNDS;
        }
        
        printf("[Validate] Section %d: Type=0x%X, Offset=%lu, Size=%lu\n", 
               i, entry.type, entry.data_offset, entry.data_size);
    }

    // 3. 푸터 해시 검증
    // 전체 파일에서 푸터(마지막 32바이트)를 제외한 부분의 해시를 계산
    fseek(fp, 0, SEEK_SET);
    
    xxh3_state_t hash_state;
    xxh3_reset(&hash_state);
    
    uint8_t* read_buf = malloc(ETVB_BUFFER_SIZE);
    if (!read_buf) {
        fclose(fp);
        return ETVB_ERR_MEMORY;
    }
    
    size_t bytes_to_hash = file_size - sizeof(etvb_footer_t);
    size_t remaining = bytes_to_hash;
    
    while (remaining > 0) {
        size_t to_read = (remaining > ETVB_BUFFER_SIZE) ? ETVB_BUFFER_SIZE : remaining;
        size_t read_count = fread(read_buf, 1, to_read, fp);
        if (read_count != to_read) {
            free(read_buf);
            fclose(fp);
            return ETVB_ERR_IO;
        }
        xxh3_update(&hash_state, read_buf, read_count);
        remaining -= read_count;
    }
    free(read_buf);
    
    uint8_t calc_hash[16];
    xxh3_digest(&hash_state, calc_hash);
    
    // 저장된 푸터 읽기
    etvb_footer_t stored_footer;
    if (fread(&stored_footer, sizeof(stored_footer), 1, fp) != 1) {
        fclose(fp);
        return ETVB_ERR_IO;
    }
    
    if (memcmp(calc_hash, stored_footer.xxh3_hash, 16) != 0) {
        printf("[Validate] Error: Hash mismatch! File may be corrupted.\n");
        fclose(fp);
        return ETVB_ERR_CHECKSUM;
    }
    
    printf("[Validate] Integrity: OK (Hash Match)\n");

    fclose(fp);
    return ETVB_OK;
}

/* ==========================================================================
 * 7. 메인 함수 (CLI Entry Point)
 * ========================================================================== */

void print_usage(const char* prog_name) {
    printf("Usage:\n");
    printf("  %s pack <model_dir> <output.etvb> [name] [arch_id]\n", prog_name);
    printf("  %s unpack <input.etvb> <output_dir>\n", prog_name);
    printf("  %s validate <input.etvb>\n", prog_name);
    printf("\nArch IDs: 1=Gemma-2B, 2=Gemma-9B, 3=Llama-7B\n");
}

int main(int argc, char* argv[]) {
    if (argc < 2) {
        print_usage(argv[0]);
        return 1;
    }

    const char* command = argv[1];
    etvb_error_t err = ETVB_OK;

    if (strcmp(command, "pack") == 0) {
        if (argc < 4) {
            fprintf(stderr, "Error: pack requires <model_dir> and <output.etvb>\n");
            return 1;
        }
        const char* model_dir = argv[2];
        const char* output_file = argv[3];
        const char* name = (argc > 4) ? argv[4] : "Unnamed Brain";
        int arch_id = (argc > 5) ? atoi(argv[5]) : ETVB_ARCH_GEMMA_2B;
        
        printf("Packing model from '%s' to '%s'...\n", model_dir, output_file);
        err = etvb_pack(model_dir, output_file, name, (etvb_architecture_t)arch_id, ETVB_COMP_NONE, ETVB_QUANT_FP16);
        
    } else if (strcmp(command, "unpack") == 0) {
        if (argc < 4) {
            fprintf(stderr, "Error: unpack requires <input.etvb> and <output_dir>\n");
            return 1;
        }
        const char* input_file = argv[2];
        const char* output_dir = argv[3];
        
        printf("Unpacking '%s' to '%s'...\n", input_file, output_dir);
        err = etvb_unpack(input_file, output_dir, false);
        
    } else if (strcmp(command, "validate") == 0) {
        if (argc < 3) {
            fprintf(stderr, "Error: validate requires <input.etvb>\n");
            return 1;
        }
        const char* input_file = argv[2];
        
        printf("Validating '%s'...\n", input_file);
        err = etvb_validate(input_file);
        
    } else {
        print_usage(argv[0]);
        return 1;
    }

    if (err != ETVB_OK) {
        fprintf(stderr, "Error: %s (%d)\n", etvb_strerror(err), err);
        return 1;
    }

    printf("Done.\n");
    return 0;
}
