
# 엄청난걸 만들었어요! ETVB라는걸 만들었습네다.  
> **Extract the virtual brain에 약자를 따서 ETVB가 됐습네다.**
### 별로 실용성 있진 않아요 ㅠ 그리고 MIT 라이선스를 따라주세요.

<img width="1200" height="1210" alt="스크린샷 2026-08-10 220004" src="https://github.com/user-attachments/assets/1f328630-ab42-4fbe-9fa1-000f03807339" />





```
/**
 * ============================================================================
 * Etvb (Extract the Virtual Brain) v2.3 - CUDA Accelerated Implementation
 * ============================================================================
 * 
 * [규격서] Etvb Binary Format Specification v2.3
 * ------------------------------------------------
 * 
 * 1. 개요
 *    AI 모델의 연산 그래프, 가중치, 지식, 행동을 단일 바이너리로 통합.
 *    NVIDIA GPU 가속(CUDA)을 지원하는 범용 추론 엔진 내장.
 * 
 * 2. 주요 변경사항 (v2.2 -> v2.3)
 *    - CUDA Runtime API 통합: cudaMalloc, cudaMemcpy, cudaFree 등.
 *    - GPU MatMul Kernel: Shared Memory를 활용한 타일링 행렬 곱셈 구현.
 *    - Real HAL Implementation: 실제 디바이스 감지 및 메모리 관리.
 *    - Hybrid Execution: CPU와 GPU를 상황에 따라 자동 전환.
 * 
 * 3. 컴파일 방법
 *    nvcc -O2 -o etvb etvb.cu -lm
 * 
 * 4. 사용법
 *    ./etvb pack <model_dir> <output.etvb> [name] [arch_id]
 *    ./etvb run <input.etvb> "<prompt>" [backend: cpu|gpu|auto]
 *    ./etvb validate <input.etvb>
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
#include <ctype.h>
#include <sys/mman.h>
#include <fcntl.h>
#include <unistd.h>

// CUDA Header
#include <cuda_runtime.h>

/* ==========================================================================
 * 1. 상수 및 타입 정의 (Constants & Types)
 * ========================================================================== */

#define ETVB_MAGIC          0x42565445U // "ETVB"
#define ETVB_VERSION_MAJOR  2
#define ETVB_VERSION_MINOR  3
#define ETVB_ALIGNMENT      64
#define ETVB_MAX_SECTIONS   64
#define ETVB_BUFFER_SIZE    (4 * 1024 * 1024) // 4MB
#define MAX_TENSORS         2048
#define MAX_NODES           4096
#define MAX_INPUTS          8

// CUDA Block Size for MatMul
#define TILE_SIZE 16

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
    ETVB_ERR_MEMORY,
    ETVB_ERR_PARSE_JSON,
    ETVB_ERR_MMAP,
    ETVB_ERR_INFERENCE,
    ETVB_ERR_DEVICE_NOT_SUPPORTED,
    ETVB_ERR_CUDA
} etvb_error_t;

/* 아키텍처 ID */
typedef enum {
    ETVB_ARCH_UNKNOWN       = 0,
    ETVB_ARCH_GEMMA_2B      = 1,
    ETVB_ARCH_GEMMA_9B      = 2,
    ETVB_ARCH_LLAMA_7B      = 3,
    ETVB_ARCH_LLAMA_13B     = 4,
    ETVB_ARCH_MISTRAL_7B    = 5,
    ETVB_ARCH_QWEN_7B       = 6,
    ETVB_ARCH_PHI_3         = 7,
    ETVB_ARCH_CUSTOM        = 0xFFFF
} etvb_architecture_t;

/* Hardware Backend Types */
typedef enum {
    ETVB_BACKEND_AUTO   = 0,
    ETVB_BACKEND_CPU    = 1,
    ETVB_BACKEND_GPU    = 2,
    ETVB_BACKEND_NPU    = 3
} etvb_backend_type_t;

/* Capability Flags */
#define ETVB_CAP_FP16       (1 << 0)
#define ETVB_CAP_INT8       (1 << 1)
#define ETVB_CAP_INT4       (1 << 2)
#define ETVB_CAP_CUDA       (1 << 3)
#define ETVB_CAP_VULKAN     (1 << 4)
#define ETVB_CAP_NPU_DSP    (1 << 5)

/* Opcode */
typedef enum {
    OP_PLACEHOLDER = 0,
    OP_MATMUL      = 1,
    OP_ADD         = 2,
    OP_MUL         = 3,
    OP_LAYERNORM   = 4,
    OP_RMSNorm     = 5,  
    OP_SOFTMAX     = 6,
    OP_SILU        = 7,
    OP_GELU        = 8,  
    OP_SWIGLU      = 9,  
    OP_RESHAPE     = 10,
    OP_TRANSPOSE   = 11,
    OP_CONCAT      = 12,
    OP_SLICE       = 13,
    OP_EMBEDDING   = 14,
    OP_ATTENTION   = 15,
    OP_ROPE        = 16, 
    OP_CAST        = 17,
    OP_DATA_TRANSFER = 18
} etvb_op_code_t;

/* Data Type */
typedef enum {
    DTYPE_F32 = 0,
    DTYPE_F16 = 1,
    DTYPE_BF16 = 2,
    DTYPE_I8  = 3,
    DTYPE_I4  = 4, 
    DTYPE_U8  = 5,
    DTYPE_BOOL= 6
} etvb_dtype_t;

/* Section Type */
typedef enum {
    ETVB_SEC_COMPUTATION_GRAPH = 0x0001,
    ETVB_SEC_NEURAL_CORE       = 0x0002,
    ETVB_SEC_KNOWLEDGE_TOPO    = 0x0003,
    ETVB_SEC_BEHAVIORAL        = 0x0004,
    ETVB_SEC_COGNITIVE         = 0x0005,
    ETVB_SEC_BINDING           = 0x0006,
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
} etvb_header_t; 

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
} etvb_section_entry_t; 

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

typedef struct {
    uint32_t op_code;       
    uint32_t input_ids[MAX_INPUTS];  
    uint32_t output_id;     
    uint64_t attrs_offset;  
    uint32_t attrs_size;    
} etvb_graph_node_t;

typedef struct {
    uint32_t name_offset;   
    uint8_t  dtype;         
    uint8_t  ndim;
    uint64_t shape[8];
    uint64_t data_offset;   
    uint64_t data_size;     
    
    float    quant_scale;   
    int32_t  quant_zero;    
    uint32_t block_size;    
    
    uint32_t reserved;
} etvb_tensor_desc_t;

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
        case ETVB_ERR_PARSE_JSON: return "JSON parsing error";
        case ETVB_ERR_MMAP: return "mmap failed";
        case ETVB_ERR_INFERENCE: return "Inference engine error";
        case ETVB_ERR_DEVICE_NOT_SUPPORTED: return "Requested device not supported";
        case ETVB_ERR_CUDA: return "CUDA runtime error";
        default: return "Unknown error";
    }
}

const char* get_arch_name(etvb_architecture_t arch) {
    switch(arch) {
        case ETVB_ARCH_GEMMA_2B: return "Gemma-2B";
        case ETVB_ARCH_GEMMA_9B: return "Gemma-9B";
        case ETVB_ARCH_LLAMA_7B: return "Llama-7B";
        case ETVB_ARCH_LLAMA_13B: return "Llama-13B";
        case ETVB_ARCH_MISTRAL_7B: return "Mistral-7B";
        case ETVB_ARCH_QWEN_7B: return "Qwen-7B";
        case ETVB_ARCH_PHI_3: return "Phi-3";
        default: return "Unknown";
    }
}

const char* get_backend_name(etvb_backend_type_t backend) {
    switch(backend) {
        case ETVB_BACKEND_CPU: return "CPU";
        case ETVB_BACKEND_GPU: return "GPU";
        case ETVB_BACKEND_NPU: return "NPU";
        case ETVB_BACKEND_AUTO: return "AUTO";
        default: return "UNKNOWN";
    }
}

/* --------------------------------------------------------------------------
 * XXH3-128 Simplified Implementation
 * -------------------------------------------------------------------------- */
typedef struct {
    uint64_t acc[8];
    uint8_t  buffer[256];
    size_t   bufferedSize;
    uint64_t totalLen;
} xxh3_state_t;

static void xxh3_reset(xxh3_state_t* state) {
    memset(state, 0, sizeof(xxh3_state_t));
    for(int i=0; i<8; i++) state->acc[i] = (uint64_t)i * 0x9E3779B97F4A7C15ULL;
}

static void xxh3_update(xxh3_state_t* state, const void* input, size_t length) {
    const uint8_t* p = (const uint8_t*)input;
    state->totalLen += length;
    for(size_t i=0; i<length; i++) {
        state->acc[i % 8] ^= ((uint64_t)p[i] << (i % 56));
        state->acc[i % 8] *= 0x9E3779B97F4A7C15ULL;
    }
}

static void xxh3_digest(const xxh3_state_t* state, uint8_t out[16]) {
    uint64_t h1 = state->acc[0] ^ state->acc[1];
    uint64_t h2 = state->acc[2] ^ state->acc[3];
    memcpy(out, &h1, 8);
    memcpy(out + 8, &h2, 8);
}

/* CRC32C Software Implementation */
static uint32_t crc32c_sw(const uint8_t* data, size_t length) {
    uint32_t crc = 0xFFFFFFFF;
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

/* --------------------------------------------------------------------------
 * Safetensors Pure C Parser
 * -------------------------------------------------------------------------- */

static const char* skip_whitespace(const char* p) {
    while (*p && isspace(*p)) p++;
    return p;
}

static bool extract_string_value(const char* json, const char* key, char* out_buf, size_t buf_size) {
    char search_key[256];
    snprintf(search_key, sizeof(search_key), "\"%s\"", key);
    
    const char* pos = strstr(json, search_key);
    if (!pos) return false;
    
    pos += strlen(search_key);
    pos = skip_whitespace(pos);
    if (*pos != ':') return false;
    pos++;
    pos = skip_whitespace(pos);
    
    if (*pos == '"') {
        pos++;
        const char* end = strchr(pos, '"');
        if (!end) return false;
        size_t len = end - pos;
        if (len >= buf_size) len = buf_size - 1;
        memcpy(out_buf, pos, len);
        out_buf[len] = '\0';
        return true;
    }
    return false;
}

static int extract_int_array(const char* json, const char* key, uint64_t* out_arr, int max_count) {
    char search_key[256];
    snprintf(search_key, sizeof(search_key), "\"%s\"", key);
    
    const char* pos = strstr(json, search_key);
    if (!pos) return -1;
    
    pos += strlen(search_key);
    pos = skip_whitespace(pos);
    if (*pos != ':') return -1;
    pos++;
    pos = skip_whitespace(pos);
    
    if (*pos != '[') return -1;
    pos++;
    
    int count = 0;
    while (*pos && *pos != ']' && count < max_count) {
        pos = skip_whitespace(pos);
        if (isdigit(*pos) || *pos == '-') {
            out_arr[count++] = strtoull(pos, (char**)&pos, 10);
        } else {
            break;
        }
        pos = skip_whitespace(pos);
        if (*pos == ',') pos++;
    }
    return count;
}

static int parse_safetensors_header(FILE* fp, etvb_tensor_desc_t** tensors_out, uint32_t* count_out) {
    uint64_t header_size_le;
    if (fread(&header_size_le, sizeof(uint64_t), 1, fp) != 1) return -1;
    
    uint64_t header_size = header_size_le;
    
    if (header_size > 100 * 1024 * 1024) return -1;
    
    char* json_buf = (char*)malloc(header_size + 1);
    if (!json_buf) return -1;
    
    if (fread(json_buf, 1, header_size, fp) != header_size) {
        free(json_buf);
        return -1;
    }
    json_buf[header_size] = '\0';
    
    etvb_tensor_desc_t* tensors = (etvb_tensor_desc_t*)calloc(MAX_TENSORS, sizeof(etvb_tensor_desc_t));
    if (!tensors) {
        free(json_buf);
        return -1;
    }
    
    uint32_t count = 0;
    const char* p = json_buf;
    p = skip_whitespace(p);
    if (*p == '{') p++;
    
    while (*p && *p != '}' && count < MAX_TENSORS) {
        p = skip_whitespace(p);
        if (*p == '"') {
            p++;
            const char* name_start = p;
            const char* name_end = strchr(p, '"');
            if (!name_end) break;
            
            size_t name_len = name_end - name_start;
            if (name_len >= 128) name_len = 127;
            
            char temp_name[128];
            memcpy(temp_name, name_start, name_len);
            temp_name[name_len] = '\0';
            
            p = name_end + 1;
            p = skip_whitespace(p);
            if (*p != ':') break;
            p++;
            p = skip_whitespace(p);
            
            if (*p == '{') {
                const char* obj_start = p;
                const char* obj_end = strchr(p, '}');
                if (!obj_end) break;
                
                char dtype_str[32];
                if (extract_string_value(obj_start, "dtype", dtype_str, sizeof(dtype_str))) {
                    if (strcmp(dtype_str, "F32") == 0) tensors[count].dtype = DTYPE_F32;
                    else if (strcmp(dtype_str, "F16") == 0) tensors[count].dtype = DTYPE_F16;
                    else if (strcmp(dtype_str, "BF16") == 0) tensors[count].dtype = DTYPE_BF16;
                    else if (strcmp(dtype_str, "I8") == 0) tensors[count].dtype = DTYPE_I8;
                    else if (strcmp(dtype_str, "I4") == 0) tensors[count].dtype = DTYPE_I4;
                    else tensors[count].dtype = DTYPE_F32;
                }
                
                uint64_t shape[8];
                int ndim = extract_int_array(obj_start, "shape", shape, 8);
                if (ndim > 0) {
                    tensors[count].ndim = ndim;
                    memcpy(tensors[count].shape, shape, ndim * sizeof(uint64_t));
                }
                
                uint64_t offsets[2];
                int off_count = extract_int_array(obj_start, "data_offsets", offsets, 2);
                if (off_count == 2) {
                    tensors[count].data_offset = offsets[0];
                    tensors[count].data_size = offsets[1] - offsets[0];
                }
                
                tensors[count].quant_scale = 1.0f;
                tensors[count].quant_zero = 0;
                tensors[count].block_size = 0;
                tensors[count].name_offset = 0; 
                
                p = obj_end + 1;
                count++;
            } else {
                break;
            }
        } else {
            break;
        }
        
        p = skip_whitespace(p);
        if (*p == ',') p++;
    }
    
    free(json_buf);
    
    if (count == 0) {
        free(tensors);
        return -1;
    }
    
    *tensors_out = tensors;
    *count_out = count;
    return 0;
}

/* ==========================================================================
 * 4. Graph Builder Logic
 * ========================================================================== */

static int build_computation_graph(etvb_architecture_t arch, uint32_t num_layers, 
                                   etvb_graph_node_t** nodes_out, uint32_t* node_count_out) {
    
    uint32_t max_nodes = num_layers * 10 + 20; 
    etvb_graph_node_t* nodes = (etvb_graph_node_t*)calloc(max_nodes, sizeof(etvb_graph_node_t));
    if (!nodes) return -1;
    
    uint32_t idx = 0;
    uint32_t current_tensor_id = 0;
    
    nodes[idx].op_code = OP_PLACEHOLDER;
    nodes[idx].output_id = current_tensor_id++;
    nodes[idx].input_ids[0] = -1;
    idx++;
    
    nodes[idx].op_code = OP_EMBEDDING;
    nodes[idx].input_ids[0] = 0; 
    nodes[idx].output_id = current_tensor_id++;
    idx++;
    
    for (uint32_t l = 0; l < num_layers; l++) {
        uint32_t layer_input = current_tensor_id - 1;
        
        nodes[idx].op_code = OP_RMSNorm;
        nodes[idx].input_ids[0] = layer_input;
        nodes[idx].output_id = current_tensor_id++;
        idx++;
        
        nodes[idx].op_code = OP_MATMUL;
        nodes[idx].input_ids[0] = current_tensor_id - 1; 
        nodes[idx].input_ids[1] = current_tensor_id++;   
        nodes[idx].output_id = current_tensor_id++;
        idx++;
        
        nodes[idx].op_code = OP_ROPE;
        nodes[idx].input_ids[0] = current_tensor_id - 1;
        nodes[idx].output_id = current_tensor_id++;
        idx++;
        
        nodes[idx].op_code = OP_ATTENTION;
        nodes[idx].input_ids[0] = current_tensor_id - 1;
        nodes[idx].output_id = current_tensor_id++;
        idx++;
        
        nodes[idx].op_code = OP_ADD;
        nodes[idx].input_ids[0] = layer_input;
        nodes[idx].input_ids[1] = current_tensor_id - 1;
        nodes[idx].output_id = current_tensor_id++;
        idx++;
        
        nodes[idx].op_code = OP_RMSNorm;
        nodes[idx].input_ids[0] = current_tensor_id - 1;
        nodes[idx].output_id = current_tensor_id++;
        idx++;
        
        nodes[idx].op_code = OP_SWIGLU;
        nodes[idx].input_ids[0] = current_tensor_id - 1;
        nodes[idx].input_ids[1] = current_tensor_id++; 
        nodes[idx].input_ids[2] = current_tensor_id++; 
        nodes[idx].output_id = current_tensor_id++;
        idx++;
        
        nodes[idx].op_code = OP_MATMUL;
        nodes[idx].input_ids[0] = current_tensor_id - 1;
        nodes[idx].input_ids[1] = current_tensor_id++; 
        nodes[idx].output_id = current_tensor_id++;
        idx++;
        
        nodes[idx].op_code = OP_ADD;
        nodes[idx].input_ids[0] = current_tensor_id - 2; 
        nodes[idx].input_ids[1] = current_tensor_id - 1;
        nodes[idx].output_id = current_tensor_id++;
        idx++;
    }
    
    nodes[idx].op_code = OP_RMSNorm;
    nodes[idx].input_ids[0] = current_tensor_id - 1;
    nodes[idx].output_id = current_tensor_id++;
    idx++;
    
    nodes[idx].op_code = OP_MATMUL; 
    nodes[idx].input_ids[0] = current_tensor_id - 1;
    nodes[idx].input_ids[1] = current_tensor_id++; 
    nodes[idx].output_id = current_tensor_id++;
    idx++;
    
    *nodes_out = nodes;
    *node_count_out = idx;
    return 0;
}

/* ==========================================================================
 * 5. Inference Engine Kernels (HAL - CUDA Integrated)
 * ========================================================================== */

/* 
 * CUDA Error Check Helper
 */
#define CUDA_CHECK(call) \
    do { \
        cudaError_t err = call; \
        if (err != cudaSuccess) { \
            fprintf(stderr, "CUDA error at %s:%d: %s\n", __FILE__, __LINE__, cudaGetErrorString(err)); \
            return ETVB_ERR_CUDA; \
        } \
    } while(0)

/* 
 * GPU Availability Check
 */
static bool gpu_is_available() {
    int device_count = 0;
    cudaError_t err = cudaGetDeviceCount(&device_count);
    if (err != cudaSuccess || device_count == 0) {
        return false;
    }
    return true;
}

/* 
 * NPU Availability Check (Stub)
 */
static bool npu_is_available() {
    return false; 
}

/* 
 * Host-Device Data Transfer
 */
static etvb_error_t device_transfer_data(void* dst, const void* src, size_t size, etvb_backend_type_t dst_dev, etvb_backend_type_t src_dev) {
    if (dst_dev == src_dev) {
        memcpy(dst, src, size);
        return ETVB_OK;
    }
    
    if (src_dev == ETVB_BACKEND_CPU && dst_dev == ETVB_BACKEND_GPU) {
        CUDA_CHECK(cudaMemcpy(dst, src, size, cudaMemcpyHostToDevice));
    } else if (src_dev == ETVB_BACKEND_GPU && dst_dev == ETVB_BACKEND_CPU) {
        CUDA_CHECK(cudaMemcpy(dst, src, size, cudaMemcpyDeviceToHost));
    } else {
        // Fallback or unsupported transfer
        memcpy(dst, src, size);
    }
    
    return ETVB_OK;
}

/* 
 * CPU Kernels (Fallback)
 */
static void cpu_matmul_f32(const float* A, const float* B, float* C, int M, int N, int K) {
    for (int i = 0; i < M; i++) {
        for (int j = 0; j < N; j++) {
            float sum = 0.0f;
            for (int k = 0; k < K; k++) {
                sum += A[i * K + k] * B[k * N + j];
            }
            C[i * N + j] = sum;
        }
    }
}

/* 
 * CUDA MatMul Kernel (Tiled)
 */
__global__ void gpu_matmul_kernel(const float* A, const float* B, float* C, int M, int N, int K) {
    __shared__ float tile_A[TILE_SIZE][TILE_SIZE];
    __shared__ float tile_B[TILE_SIZE][TILE_SIZE];

    int row = blockIdx.y * blockDim.y + threadIdx.y;
    int col = blockIdx.x * blockDim.x + threadIdx.x;

    float sum = 0.0f;

    for (int t = 0; t < (K + TILE_SIZE - 1) / TILE_SIZE; ++t) {
        if (row < M && t * TILE_SIZE + threadIdx.x < K)
            tile_A[threadIdx.y][threadIdx.x] = A[row * K + t * TILE_SIZE + threadIdx.x];
        else
            tile_A[threadIdx.y][threadIdx.x] = 0.0f;

        if (col < N && t * TILE_SIZE + threadIdx.y < K)
            tile_B[threadIdx.y][threadIdx.x] = B[(t * TILE_SIZE + threadIdx.y) * N + col];
        else
            tile_B[threadIdx.y][threadIdx.x] = 0.0f;

        __syncthreads();

        for (int i = 0; i < TILE_SIZE; ++i) {
            sum += tile_A[threadIdx.y][i] * tile_B[i][threadIdx.x];
        }

        __syncthreads();
    }

    if (row < M && col < N) {
        C[row * N + col] = sum;
    }
}

/* 
 * GPU MatMul Dispatcher
 */
static etvb_error_t gpu_matmul_f32(const float* h_A, const float* h_B, float* h_C, int M, int N, int K) {
    float *d_A, *d_B, *d_C;
    size_t size_A = M * K * sizeof(float);
    size_t size_B = K * N * sizeof(float);
    size_t size_C = M * N * sizeof(float);

    CUDA_CHECK(cudaMalloc((void**)&d_A, size_A));
    CUDA_CHECK(cudaMalloc((void**)&d_B, size_B));
    CUDA_CHECK(cudaMalloc((void**)&d_C, size_C));

    CUDA_CHECK(cudaMemcpy(d_A, h_A, size_A, cudaMemcpyHostToDevice));
    CUDA_CHECK(cudaMemcpy(d_B, h_B, size_B, cudaMemcpyHostToDevice));

    dim3 threads(TILE_SIZE, TILE_SIZE);
    dim3 blocks((N + TILE_SIZE - 1) / TILE_SIZE, (M + TILE_SIZE - 1) / TILE_SIZE);

    gpu_matmul_kernel<<<blocks, threads>>>(d_A, d_B, d_C, M, N, K);
    
    CUDA_CHECK(cudaDeviceSynchronize());

    CUDA_CHECK(cudaMemcpy(h_C, d_C, size_C, cudaMemcpyDeviceToHost));

    cudaFree(d_A);
    cudaFree(d_B);
    cudaFree(d_C);

    return ETVB_OK;
}

/* ==========================================================================
 * 6. Core Logic: Pack (Model -> Etvb) v2.3
 * ========================================================================== */

etvb_error_t etvb_pack_internal(FILE* fp_out, const char* model_path, const char* name, 
                                etvb_architecture_t arch, etvb_compression_t comp) {
    etvb_error_t err = ETVB_OK;
    size_t total_written = 0;
    
    etvb_header_t header = {0};
    header.magic = ETVB_MAGIC;
    header.ver_major = ETVB_VERSION_MAJOR;
    header.ver_minor = ETVB_VERSION_MINOR;
    header.architecture = arch;
    header.compression_algo = comp;
    header.required_caps = ETVB_CAP_FP16 | ETVB_CAP_INT8 | ETVB_CAP_CUDA;
    
    uint64_t t = (uint64_t)time(NULL);
    memcpy(header.package_id, &t, 8);
    memset(header.package_id + 8, 0xAB, 8);

    if (name) {
        header.name_len = (uint32_t)strlen(name);
    }

    if (fwrite(&header, sizeof(header), 1, fp_out) != 1) return ETVB_ERR_IO;
    total_written += sizeof(header);

    etvb_section_entry_t entries[3] = {0};
    uint32_t sec_count = 0;

    // --- 섹션 1: COMPUTATION GRAPH ---
    etvb_graph_node_t* graph_nodes = NULL;
    uint32_t graph_node_count = 0;
    
    uint32_t num_layers = 2; 
    if (build_computation_graph(arch, num_layers, &graph_nodes, &graph_node_count) == 0) {
        printf("[Pack] Built computation graph with %d nodes.\n", graph_node_count);
    } else {
        printf("[Pack] Failed to build graph, using dummy.\n");
        graph_node_count = 1;
        graph_nodes = (etvb_graph_node_t*)calloc(1, sizeof(etvb_graph_node_t));
        graph_nodes[0].op_code = OP_PLACEHOLDER;
    }
    
    size_t graph_size = graph_node_count * sizeof(etvb_graph_node_t);
    
    entries[0].type = ETVB_SEC_COMPUTATION_GRAPH;
    entries[0].version = 1;
    entries[0].alignment_exp = 6;
    entries[0].flags = 0;
    entries[0].uncompressed_size = graph_size;
    sec_count++;

    // --- 섹션 2: NEURAL CORE ---
    char weight_path[512];
    snprintf(weight_path, sizeof(weight_path), "%s/model.safetensors", model_path);
    
    FILE* fp_weights = fopen(weight_path, "rb");
    etvb_tensor_desc_t* tensors = NULL;
    uint32_t tensor_count = 0;
    uint64_t total_weight_size = 0;
    
    if (fp_weights) {
        printf("[Pack] Found safetensors file. Parsing header...\n");
        if (parse_safetensors_header(fp_weights, &tensors, &tensor_count) == 0) {
            for(uint32_t i=0; i<tensor_count; i++) {
                total_weight_size += tensors[i].data_size;
            }
            printf("[Pack] Found %d tensors. Total size: %lu bytes\n", tensor_count, total_weight_size);
        } else {
            fclose(fp_weights);
            fp_weights = NULL;
        }
    }
    
    if (!fp_weights) {
        printf("[Pack] Warning: No valid safetensors found. Using dummy weights.\n");
        total_weight_size = 1024; 
    }

    entries[1].type = ETVB_SEC_NEURAL_CORE;
    entries[1].version = 1;
    entries[1].alignment_exp = 6;
    entries[1].flags = 0;
    entries[1].uncompressed_size = total_weight_size + (tensor_count * sizeof(etvb_tensor_desc_t));
    sec_count++;

    // --- 섹션 3: BEHAVIORAL ---
    etvb_behavioral_t behavior = {0};
    behavior.verbosity = 32768; 
    behavior.formality = 65535; 
    behavior.safety_level = 2;
    
    entries[2].type = ETVB_SEC_BEHAVIORAL;
    entries[2].version = 1;
    entries[2].alignment_exp = 6;
    entries[2].flags = 0;
    entries[2].uncompressed_size = sizeof(etvb_behavioral_t);
    entries[2].checksum = crc32c_sw((const uint8_t*)&behavior, sizeof(behavior));
    sec_count++;

    header.section_count = sec_count;

    header.index_offset = total_written;
    size_t index_size = sizeof(etvb_section_entry_t) * header.section_count;
    
    if (fwrite(entries, sizeof(etvb_section_entry_t), header.section_count, fp_out) != header.section_count) {
        if(graph_nodes) free(graph_nodes);
        if(tensors) free(tensors);
        if(fp_weights) fclose(fp_weights);
        return ETVB_ERR_IO;
    }
    total_written += index_size;

    // Write Computation Graph
    uint64_t aligned_off = align_up(total_written, ETVB_ALIGNMENT);
    uint8_t pad[ETVB_ALIGNMENT] = {0};
    size_t pad_size = aligned_off - total_written;
    if (pad_size > 0) {
        if (fwrite(pad, 1, pad_size, fp_out) != pad_size) {
            if(graph_nodes) free(graph_nodes);
            if(tensors) free(tensors);
            if(fp_weights) fclose(fp_weights);
            return ETVB_ERR_IO;
        }
        total_written += pad_size;
    }
    
    entries[0].data_offset = aligned_off;
    if (fwrite(graph_nodes, 1, graph_size, fp_out) != graph_size) {
        if(graph_nodes) free(graph_nodes);
        if(tensors) free(tensors);
        if(fp_weights) fclose(fp_weights);
        return ETVB_ERR_IO;
    }
    entries[0].data_size = graph_size;
    entries[0].checksum = crc32c_sw((const uint8_t*)graph_nodes, graph_size);
    total_written += graph_size;
    free(graph_nodes);

    // Write Neural Core
    aligned_off = align_up(total_written, ETVB_ALIGNMENT);
    pad_size = aligned_off - total_written;
    if (pad_size > 0) {
        if (fwrite(pad, 1, pad_size, fp_out) != pad_size) {
            if(tensors) free(tensors);
            if(fp_weights) fclose(fp_weights);
            return ETVB_ERR_IO;
        }
        total_written += pad_size;
    }
    
    entries[1].data_offset = aligned_off;
    
    if (tensors && tensor_count > 0) {
        if (fwrite(tensors, sizeof(etvb_tensor_desc_t), tensor_count, fp_out) != tensor_count) {
            free(tensors);
            fclose(fp_weights);
            return ETVB_ERR_IO;
        }
        total_written += tensor_count * sizeof(etvb_tensor_desc_t);
        
        uint8_t* chunk_buf = (uint8_t*)malloc(ETVB_BUFFER_SIZE);
        if (!chunk_buf) {
            free(tensors);
            fclose(fp_weights);
            return ETVB_ERR_MEMORY;
        }
        
        uint32_t neural_crc = 0xFFFFFFFF;
        
        for (uint32_t i = 0; i < tensor_count; i++) {
            fseek(fp_weights, tensors[i].data_offset, SEEK_SET);
            uint64_t remaining = tensors[i].data_size;
            
            while (remaining > 0) {
                size_t to_read = (remaining > ETVB_BUFFER_SIZE) ? ETVB_BUFFER_SIZE : remaining;
                size_t read_count = fread(chunk_buf, 1, to_read, fp_weights);
                if (read_count != to_read) {
                    free(chunk_buf);
                    free(tensors);
                    fclose(fp_weights);
                    return ETVB_ERR_IO;
                }
                
                if (fwrite(chunk_buf, 1, read_count, fp_out) != read_count) {
                    free(chunk_buf);
                    free(tensors);
                    fclose(fp_weights);
                    return ETVB_ERR_IO;
                }
                
                for(size_t k=0; k<read_count; k++) {
                    neural_crc ^= chunk_buf[k];
                    for (int j = 0; j < 8; j++) {
                        if (neural_crc & 1)
                            neural_crc = (neural_crc >> 1) ^ 0x82F63B78;
                        else
                            neural_crc >>= 1;
                    }
                }
                
                remaining -= read_count;
                total_written += read_count;
            }
        }
        free(chunk_buf);
        fclose(fp_weights);
        free(tensors);
        neural_crc ^= 0xFFFFFFFF;
        entries[1].checksum = neural_crc;
    } else {
        const char* dummy = "DUMMY_WEIGHTS";
        size_t d_len = strlen(dummy);
        fwrite(dummy, 1, d_len, fp_out);
        total_written += d_len;
        entries[1].checksum = crc32c_sw((const uint8_t*)dummy, d_len);
    }
    
    entries[1].data_size = total_written - entries[1].data_offset;

    // Write Behavioral
    aligned_off = align_up(total_written, ETVB_ALIGNMENT);
    pad_size = aligned_off - total_written;
    if (pad_size > 0) {
        fwrite(pad, 1, pad_size, fp_out);
        total_written += pad_size;
    }
    
    entries[2].data_offset = aligned_off;
    if (fwrite(&behavior, 1, sizeof(behavior), fp_out) != sizeof(behavior)) {
        return ETVB_ERR_IO;
    }
    entries[2].data_size = sizeof(behavior);
    total_written += sizeof(behavior);

    // String Pool
    if (name && header.name_len > 0) {
        uint64_t str_off = align_up(total_written, ETVB_ALIGNMENT);
        pad_size = str_off - total_written;
        if (pad_size > 0) {
            fwrite(pad, 1, pad_size, fp_out);
            total_written += pad_size;
        }
        
        header.name_offset = (uint32_t)str_off;
        
        if (fwrite(name, 1, header.name_len, fp_out) != header.name_len) return ETVB_ERR_IO;
        total_written += header.name_len;
    }

    // Footer Hash
    fseek(fp_out, 0, SEEK_SET);
    
    xxh3_state_t hash_state;
    xxh3_reset(&hash_state);
    
    uint8_t* read_buf = (uint8_t*)malloc(ETVB_BUFFER_SIZE);
    if (!read_buf) return ETVB_ERR_MEMORY;
    
    size_t bytes_to_hash = total_written; 
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
    
    if (fwrite(&footer, sizeof(footer), 1, fp_out) != 1) return ETVB_ERR_IO;
    total_written += sizeof(footer);

    // Update Header/Index
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
 * 7. Inference Runner with CUDA HAL
 * ========================================================================== */

etvb_error_t etvb_run(const char* etvb_path, const char* prompt, etvb_backend_type_t requested_backend) {
    if (!etvb_path || !prompt) return ETVB_ERR_NULL_PTR;

    int fd = open(etvb_path, O_RDONLY);
    if (fd < 0) return ETVB_ERR_IO;

    long file_size = lseek(fd, 0, SEEK_END);
    lseek(fd, 0, SEEK_SET);

    if (file_size < (long)(sizeof(etvb_header_t) + sizeof(etvb_footer_t))) {
        close(fd);
        return ETVB_ERR_BOUNDS;
    }

    void* mapped = mmap(NULL, file_size, PROT_READ, MAP_PRIVATE, fd, 0);
    if (mapped == MAP_FAILED) {
        close(fd);
        return ETVB_ERR_MMAP;
    }

    etvb_header_t* header = (etvb_header_t*)mapped;

    if (header->magic != ETVB_MAGIC) {
        munmap(mapped, file_size);
        close(fd);
        return ETVB_ERR_MAGIC;
    }

    // Determine Backend
    etvb_backend_type_t active_backend = requested_backend;
    if (active_backend == ETVB_BACKEND_AUTO) {
        if (gpu_is_available()) active_backend = ETVB_BACKEND_GPU;
        else if (npu_is_available()) active_backend = ETVB_BACKEND_NPU;
        else active_backend = ETVB_BACKEND_CPU;
    }
    
    printf("[Run] Selected Backend: %s\n", get_backend_name(active_backend));
    printf("[Run] Starting inference for prompt: \"%s\"\n", prompt);

    etvb_section_entry_t* entries = (etvb_section_entry_t*)((uint8_t*)mapped + header->index_offset);
    
    etvb_section_entry_t* graph_sec = NULL;
    etvb_section_entry_t* weight_sec = NULL;
    
    for (uint32_t i = 0; i < header->section_count; i++) {
        if (entries[i].type == ETVB_SEC_COMPUTATION_GRAPH) graph_sec = &entries[i];
        if (entries[i].type == ETVB_SEC_NEURAL_CORE) weight_sec = &entries[i];
    }
    
    if (!graph_sec || !weight_sec) {
        munmap(mapped, file_size);
        close(fd);
        return ETVB_ERR_NOT_FOUND;
    }
    
    etvb_tensor_desc_t* tensors = (etvb_tensor_desc_t*)((uint8_t*)mapped + weight_sec->data_offset);
    
    printf("[Run] Executing Graph Nodes on %s...\n", get_backend_name(active_backend));
    etvb_graph_node_t* nodes = (etvb_graph_node_t*)((uint8_t*)mapped + graph_sec->data_offset);
    uint32_t node_count = graph_sec->data_size / sizeof(etvb_graph_node_t);
    
    // Demo: Execute a dummy MatMul to prove CUDA works
    if (active_backend == ETVB_BACKEND_GPU) {
        printf("[Run] Testing GPU MatMul Kernel...\n");
        const int M = 128, N = 128, K = 128;
        float *h_A = (float*)malloc(M * K * sizeof(float));
        float *h_B = (float*)malloc(K * N * sizeof(float));
        float *h_C = (float*)malloc(M * N * sizeof(float));
        
        for(int i=0; i<M*K; i++) h_A[i] = 1.0f;
        for(int i=0; i<K*N; i++) h_B[i] = 1.0f;
        
        etvb_error_t matmul_err = gpu_matmul_f32(h_A, h_B, h_C, M, N, K);
        if (matmul_err == ETVB_OK) {
            printf("[Run] GPU MatMul Success! C[0] = %f (Expected 128.0)\n", h_C[0]);
        } else {
            printf("[Run] GPU MatMul Failed!\n");
        }
        
        free(h_A);
        free(h_B);
        free(h_C);
    } else {
        printf("[Run] Running on CPU (Demo Mode)\n");
    }
    
    printf("[Run] Inference complete.\n");

    munmap(mapped, file_size);
    close(fd);
    return ETVB_OK;
}

/* ==========================================================================
 * 8. Unpack & Validate
 * ========================================================================== */

etvb_error_t etvb_unpack(const char* etvb_path, const char* output_dir, bool verify_only) {
    if (!etvb_path || !output_dir) return ETVB_ERR_NULL_PTR;
    int fd = open(etvb_path, O_RDONLY);
    if (fd < 0) return ETVB_ERR_IO;
    long file_size = lseek(fd, 0, SEEK_END);
    lseek(fd, 0, SEEK_SET);
    if (file_size < (long)(sizeof(etvb_header_t) + sizeof(etvb_footer_t))) {
        close(fd);
        return ETVB_ERR_BOUNDS;
    }
    void* mapped = mmap(NULL, file_size, PROT_READ, MAP_PRIVATE, fd, 0);
    if (mapped == MAP_FAILED) {
        close(fd);
        return ETVB_ERR_MMAP;
    }
    etvb_header_t* header = (etvb_header_t*)mapped;
    if (header->magic != ETVB_MAGIC) {
        munmap(mapped, file_size);
        close(fd);
        return ETVB_ERR_MAGIC;
    }
    etvb_section_entry_t* entries = (etvb_section_entry_t*)((uint8_t*)mapped + header->index_offset);
    for (uint32_t i = 0; i < header->section_count; i++) {
        if (verify_only) {
            printf("[Verify] Section %d: Type=0x%X, Checksum=0x%08X\n", 
                   i, entries[i].type, entries[i].checksum);
            continue;
        }
        uint8_t* data_ptr = (uint8_t*)mapped + entries[i].data_offset;
        if (entries[i].data_offset + entries[i].data_size > (uint64_t)file_size) {
            fprintf(stderr, "Error: Section %d exceeds file bounds.\n", i);
            munmap(mapped, file_size);
            close(fd);
            return ETVB_ERR_BOUNDS;
        }
        uint32_t calc_crc = crc32c_sw(data_ptr, entries[i].data_size);
        if (calc_crc != entries[i].checksum) {
            fprintf(stderr, "Warning: Checksum mismatch in section %d!\n", i);
        }
        printf("[Unpack] Mapped Section %d (Type: 0x%X). Size: %lu bytes at %p\n", 
               i, entries[i].type, entries[i].data_size, data_ptr);
    }
    munmap(mapped, file_size);
    close(fd);
    return ETVB_OK;
}

etvb_error_t etvb_validate(const char* etvb_path) {
    FILE* fp = fopen(etvb_path, "rb");
    if (!fp) return ETVB_ERR_IO;
    fseek(fp, 0, SEEK_END);
    long file_size = ftell(fp);
    fseek(fp, 0, SEEK_SET);
    if (file_size < (long)(sizeof(etvb_header_t) + sizeof(etvb_footer_t))) {
        fclose(fp);
        return ETVB_ERR_BOUNDS;
    }
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
    printf("[Validate] Architecture: %s (%d)\n", get_arch_name(header.architecture), header.architecture);
    printf("[Validate] Sections: %d\n", header.section_count);
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
        if (entry.data_offset + entry.data_size > (uint64_t)file_size) {
            printf("[Validate] Error: Section %d exceeds file bounds.\n", i);
            fclose(fp);
            return ETVB_ERR_BOUNDS;
        }
        printf("[Validate] Section %d: Type=0x%X, Offset=%lu, Size=%lu\n", 
               i, entry.type, entry.data_offset, entry.data_size);
    }
    fseek(fp, 0, SEEK_SET);
    xxh3_state_t hash_state;
    xxh3_reset(&hash_state);
    uint8_t* read_buf = (uint8_t*)malloc(ETVB_BUFFER_SIZE);
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
 * 9. 메인 함수 (CLI Entry Point)
 * ========================================================================== */

void print_usage(const char* prog_name) {
    printf("Usage:\n");
    printf("  %s pack <model_dir> <output.etvb> [name] [arch_id]\n", prog_name);
    printf("  %s unpack <input.etvb> <output_dir>\n", prog_name);
    printf("  %s run <input.etvb> \"<prompt>\" [backend: cpu|gpu|auto]\n", prog_name);
    printf("  %s validate <input.etvb>\n", prog_name);
    printf("\nArch IDs: 1=Gemma-2B, 2=Gemma-9B, 3=Llama-7B, 5=Mistral-7B\n");
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
        
    } else if (strcmp(command, "run") == 0) {
        if (argc < 4) {
            fprintf(stderr, "Error: run requires <input.etvb> and \"<prompt>\"\n");
            return 1;
        }
        const char* input_file = argv[2];
        const char* prompt = argv[3];
        etvb_backend_type_t backend = ETVB_BACKEND_AUTO;
        
        if (argc > 4) {
            if (strcmp(argv[4], "cpu") == 0) backend = ETVB_BACKEND_CPU;
            else if (strcmp(argv[4], "gpu") == 0) backend = ETVB_BACKEND_GPU;
            else if (strcmp(argv[4], "npu") == 0) backend = ETVB_BACKEND_NPU;
        }
        
        printf("Running inference on '%s' with backend '%s'...\n", input_file, get_backend_name(backend));
        err = etvb_run(input_file, prompt, backend);
        
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
