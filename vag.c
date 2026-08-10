/*
 * vag.cpp
 *
 * VAG 아카이브 라이브러리 v2 (단일 파일)
 * - 순수 C99 기반 (C++ 컴파일 가능하도록 extern "C" 사용)
 * - 압축 없음
 * - 파일 데이터 Flat 저장 + Central Directory/Footer 인덱스
 *
 * VAG v2 파일 구조:
 *
 * [file data 0][file data 1]...[file data N]
 * [central directory]
 * [footer]
 *
 * Central Directory Entry (v2):
 *   uint16 name_len
 *   char   name[name_len]
 *   uint64 data_offset
 *   uint64 data_size
 *   uint64 mtime_unix_seconds
 *   uint32 crc32
 *
 * Footer (20 bytes):
 *   uint64 central_directory_offset
 *   uint32 entry_count
 *   uint32 version (= 2)
 *   char   magic[4] = "VAG!"
 *
 * 추가 기능:
 * - 가상 폴더 트리 자동 생성 및 탐색
 * - 부분 추출(파일/폴더 패턴)
 * - 메타데이터: mtime + CRC32
 *
 * 참고:
 * - 수정 시간/디렉터리 생성은 표준 C만으로 완전히 이식 가능하지 않아
 *   POSIX/Windows 분기를 넣었습니다.
 * - 경로 보안을 위해 ".." 컴포넌트는 거부하고, 내부 이름은 정규화합니다.
 */

#if !defined(_WIN32) && !defined(_POSIX_C_SOURCE)
#define _POSIX_C_SOURCE 200809L
#endif

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <stdint.h>
#include <limits.h>
#include <errno.h>

#if defined(_WIN32)
#include <direct.h>
#endif

#include <sys/stat.h>

#define VAG_MAGIC_TEXT "VAG!"
#define VAG_MAGIC_LEN 4

/* VAG v2 footer size: offset(8) + count(4) + version(4) + magic(4) */
#define VAG_VERSION 2u
#define VAG_FOOTER_SIZE 20u

#define VAG_MAX_NAME_LEN ((uint16_t)0xFFFFu)
#define VAG_COPY_BUF_SIZE (1024u * 1024u)

typedef enum {
    VAG_OK = 0,
    VAG_ERR_NULL,
    VAG_ERR_INVALID_NAME,
    VAG_ERR_OPEN_READ,
    VAG_ERR_OPEN_WRITE,
    VAG_ERR_IO,
    VAG_ERR_NO_MEMORY,
    VAG_ERR_NOT_VAG,
    VAG_ERR_CORRUPT,
    VAG_ERR_NOT_FOUND,
    VAG_ERR_NAME_TOO_LONG,
    VAG_ERR_DUPLICATE,
    VAG_ERR_INVALID_MODE,
    VAG_ERR_TOO_LARGE,
    VAG_ERR_TOO_MANY,
    VAG_ERR_UNSUPPORTED,
    VAG_ERR_CHECKSUM,
    VAG_ERR_PATH
} vag_error;

typedef enum {
    VAG_EXTRACT_FLAG_NONE = 0,
    VAG_EXTRACT_FLAG_NO_VERIFY = 1
} vag_extract_flags;

/* Forward declaration for tree node */
typedef struct vag_tree_node vag_tree_node;

/* Central Directory entry */
typedef struct vag_entry {
    char*    name;   /* 정규화된 가상 경로 (C 문자열) */
    uint64_t offset; /* 데이터 시작 위치 */
    uint64_t size;   /* 데이터 크기 */
    uint64_t mtime;  /* Unix seconds, unknown = 0 */
    uint32_t crc32;  /* CRC32 checksum */
} vag_entry;

/* Archive handle */
typedef struct vag_archive {
    FILE* fp;

    int is_writing;
    int broken;

    vag_error last_error;

    vag_entry* entries;
    size_t count;
    size_t capacity;

    uint64_t next_data_offset;

    /* Lazy-built virtual folder tree */
    vag_tree_node* tree_root;
} vag_archive;

#ifdef __cplusplus
extern "C" {
#endif

/* ------------------------------------------------------------------ */
/* Public API                                                          */
/* ------------------------------------------------------------------ */

const char* vag_strerror(vag_error err);

/* Write API */
vag_archive* vag_open_write(const char* archive_path, vag_error* err);
vag_error vag_add_file(vag_archive* ar,
                       const char* archive_name,
                       const char* disk_path);
vag_error vag_close(vag_archive* ar);
void vag_abort(vag_archive* ar);

/* Read API */
vag_archive* vag_open_read(const char* archive_path, vag_error* err);

/* One-shot create/extract */
vag_error vag_create(const char* archive_path,
                     const char* const* file_paths,
                     const char* const* archive_names,
                     size_t file_count);

vag_error vag_extract(const char* archive_path,
                      const char* archive_name,
                      const char* output_path);

/* Partial extract */
vag_error vag_extract_partial(vag_archive* ar,
                              const char* output_dir,
                              const char* const* patterns,
                              size_t pattern_count,
                              unsigned flags);

vag_error vag_extract_partial_from_file(const char* archive_path,
                                        const char* output_dir,
                                        const char* const* patterns,
                                        size_t pattern_count,
                                        unsigned flags);

vag_error vag_extract_all(const char* archive_path,
                          const char* output_dir,
                          unsigned flags);

/* Entry query */
size_t vag_count(const vag_archive* ar);
const vag_entry* vag_get(const vag_archive* ar, size_t index);
const vag_entry* vag_find(const vag_archive* ar, const char* archive_name);

/* Virtual folder tree */
const vag_tree_node* vag_tree_root(vag_archive* ar);
const vag_tree_node* vag_tree_find_node(vag_archive* ar, const char* path);

size_t vag_tree_child_count(const vag_tree_node* node);
const vag_tree_node* vag_tree_child(const vag_tree_node* node, size_t index);
const char* vag_tree_name(const vag_tree_node* node);
int vag_tree_is_dir(const vag_tree_node* node);
size_t vag_tree_entry_index(const vag_tree_node* node);

void vag_tree_print(vag_archive* ar);

/* ------------------------------------------------------------------ */
/* Internal: misc helpers                                              */
/* ------------------------------------------------------------------ */

static int vag_is_alpha(unsigned char c)
{
    return (c >= 'A' && c <= 'Z') || (c >= 'a' && c <= 'z');
}

/*
 * 가상 경로를 정규화한다.
 * - '/'와 '\'를 '/'로 통일
 * - leading slash 제거
 * - "." 컴포넌트 제거
 * - ".." 컴포넌트가 있으면 NULL 반환(경로 순회 거부)
 * - Windows 드라이브 문자 컴포넌트 제거 (예: C:/ -> 제거)
 *
 * 반환:
 *   성공 시 malloc된 문자열
 *   경로 순회 또는 메모리 실패 시 NULL
 */
static char* vag_normalize_virtual_path(const char* path)
{
    if (!path) {
        return NULL;
    }

    size_t path_len = strlen(path);
    char* out = (char*)malloc(path_len + 1);
    if (!out) {
        return NULL;
    }

    size_t out_len = 0;
    const char* p = path;
    int first = 1;

    while (*p != '\0') {
        while (*p == '/' || *p == '\\') {
            ++p;
        }

        if (*p == '\0') {
            break;
        }

        const char* start = p;

        while (*p != '\0' && *p != '/' && *p != '\\') {
            ++p;
        }

        size_t len = (size_t)(p - start);

        if (len == 0) {
            continue;
        }

        /* Windows drive prefix 제거: C:, D: 등 */
        if (len >= 2 &&
            start[1] == ':' &&
            vag_is_alpha((unsigned char)start[0])) {
            start += 2;
            len -= 2;

            if (len == 0) {
                continue;
            }
        }

        /* "." 컴포넌트 무시 */
        if (len == 1 && start[0] == '.') {
            continue;
        }

        /* ".." 컴포넌트 거부 */
        if (len == 2 && start[0] == '.' && start[1] == '.') {
            free(out);
            return NULL;
        }

        if (!first) {
            out[out_len++] = '/';
        }

        memcpy(out + out_len, start, len);
        out_len += len;
        first = 0;
    }

    out[out_len] = '\0';
    return out;
}

static char* vag_strdup(const char* s)
{
    size_t len = strlen(s) + 1;
    char* d = (char*)malloc(len);
    if (d) {
        memcpy(d, s, len);
    }
    return d;
}

/* ------------------------------------------------------------------ */
/* Internal: 64-bit seek/tell                                          */
/* ------------------------------------------------------------------ */

static int vag_seek_set(FILE* fp, uint64_t offset)
{
#if defined(_MSC_VER)
    return _fseeki64(fp, (long long)offset, SEEK_SET);
#else
    if (offset > (uint64_t)LONG_MAX) {
        return -1;
    }
    return fseek(fp, (long)offset, SEEK_SET);
#endif
}

static uint64_t vag_tell_pos(FILE* fp)
{
#if defined(_MSC_VER)
    long long pos = _ftelli64(fp);
    return (pos < 0) ? UINT64_MAX : (uint64_t)pos;
#else
    long pos = ftell(fp);
    return (pos < 0) ? UINT64_MAX : (uint64_t)pos;
#endif
}

/* ------------------------------------------------------------------ */
/* Internal: exact IO                                                  */
/* ------------------------------------------------------------------ */

static vag_error vag_read_exact(FILE* fp, void* buf, size_t n)
{
    size_t r = fread(buf, 1, n, fp);
    if (r == n) {
        return VAG_OK;
    }
    return ferror(fp) ? VAG_ERR_IO : VAG_ERR_CORRUPT;
}

static vag_error vag_write_exact(FILE* fp, const void* buf, size_t n)
{
    size_t w = fwrite(buf, 1, n, fp);
    return (w == n) ? VAG_OK : VAG_ERR_IO;
}

/* ------------------------------------------------------------------ */
/* Internal: little-endian serialization                               */
/* ------------------------------------------------------------------ */

static void vag_put_u16(uint8_t* p, uint16_t v)
{
    int i;
    for (i = 0; i < 2; ++i) {
        p[i] = (uint8_t)((v >> (8 * i)) & 0xFFu);
    }
}

static void vag_put_u32(uint8_t* p, uint32_t v)
{
    int i;
    for (i = 0; i < 4; ++i) {
        p[i] = (uint8_t)((v >> (8 * i)) & 0xFFu);
    }
}

static void vag_put_u64(uint8_t* p, uint64_t v)
{
    int i;
    for (i = 0; i < 8; ++i) {
        p[i] = (uint8_t)((v >> (8 * i)) & 0xFFu);
    }
}

static uint16_t vag_get_u16(const uint8_t* p)
{
    uint16_t v = 0;
    int i;
    for (i = 0; i < 2; ++i) {
        v = (uint16_t)(v | ((uint16_t)p[i] << (8 * i)));
    }
    return v;
}

static uint32_t vag_get_u32(const uint8_t* p)
{
    uint32_t v = 0;
    int i;
    for (i = 0; i < 4; ++i) {
        v = (uint32_t)(v | ((uint32_t)p[i] << (8 * i)));
    }
    return v;
}

static uint64_t vag_get_u64(const uint8_t* p)
{
    uint64_t v = 0;
    int i;
    for (i = 0; i < 8; ++i) {
        v |= (uint64_t)p[i] << (8 * i);
    }
    return v;
}

/* ------------------------------------------------------------------ */
/* Internal: CRC32                                                     */
/* ------------------------------------------------------------------ */

static uint32_t vag_crc_table[256];
static int vag_crc_table_ready = 0;

static void vag_crc32_init(void)
{
    uint32_t i;

    if (vag_crc_table_ready) {
        return;
    }

    for (i = 0; i < 256u; ++i) {
        uint32_t c = i;
        int j;

        for (j = 0; j < 8; ++j) {
            if (c & 1u) {
                c = 0xEDB88320u ^ (c >> 1);
            } else {
                c = c >> 1;
            }
        }

        vag_crc_table[i] = c;
    }

    vag_crc_table_ready = 1;
}

/*
 * 표준적인 CRC32(IEEE 802.3) 계산.
 * chunk 단위로 호출 가능:
 *   crc = 0;
 *   crc = vag_crc32_update(crc, buf1, len1);
 *   crc = vag_crc32_update(crc, buf2, len2);
 */
static uint32_t vag_crc32_update(uint32_t crc, const void* data, size_t len)
{
    const unsigned char* p = (const unsigned char*)data;
    size_t i;

    vag_crc32_init();

    crc = ~crc;

    for (i = 0; i < len; ++i) {
        crc = vag_crc_table[(crc ^ p[i]) & 0xFFu] ^ (crc >> 8);
    }

    return ~crc;
}

/* ------------------------------------------------------------------ */
/* Internal: file mtime                                                */
/* ------------------------------------------------------------------ */

static uint64_t vag_get_file_mtime(const char* path)
{
    uint64_t out = 0;

#if defined(_WIN32) && defined(_MSC_VER)
    struct _stat64 st;
    if (_stat64(path, &st) == 0) {
        long long t = (long long)st.st_mtime;
        out = (t < 0) ? 0 : (uint64_t)t;
    }
#else
    struct stat st;
    if (stat(path, &st) == 0) {
        long long t = (long long)st.st_mtime;
        out = (t < 0) ? 0 : (uint64_t)t;
    }
#endif

    return out;
}

/* ------------------------------------------------------------------ */
/* Internal: directory creation                                        */
/* ------------------------------------------------------------------ */

static int vag_mkdir_one(const char* path)
{
#if defined(_WIN32)
    return _mkdir(path);
#else
    return mkdir(path, 0755);
#endif
}

static int vag_mkdirs(const char* path)
{
    if (!path || path[0] == '\0') {
        return 0;
    }

    char* tmp = vag_strdup(path);
    if (!tmp) {
        return -1;
    }

    size_t len = strlen(tmp);

    /* trailing slash 제거 */
    while (len > 1 && (tmp[len - 1] == '/' || tmp[len - 1] == '\\')) {
        tmp[--len] = '\0';
    }

    if (len == 0) {
        free(tmp);
        return 0;
    }

    /* Unix root */
    if (strcmp(tmp, "/") == 0) {
        free(tmp);
        return 0;
    }

#if defined(_WIN32)
    /* Windows drive root */
    if (len == 2 && tmp[1] == ':' && vag_is_alpha((unsigned char)tmp[0])) {
        free(tmp);
        return 0;
    }
#endif

    char* p = tmp;

#if defined(_WIN32)
    if (vag_is_alpha((unsigned char)p[0]) && p[1] == ':') {
        p += 2;
    }
#endif

    if (*p == '/' || *p == '\\') {
        ++p;
    }

    for (; *p != '\0'; ++p) {
        if (*p == '/' || *p == '\\') {
            *p = '\0';

            if (vag_mkdir_one(tmp) != 0 && errno != EEXIST) {
                free(tmp);
                return -1;
            }

            *p = '/';
        }
    }

    if (vag_mkdir_one(tmp) != 0 && errno != EEXIST) {
        free(tmp);
        return -1;
    }

    free(tmp);
    return 0;
}

/*
 * output_dir + internal_name 형태의 안전한 전체 출력 경로를 만든다.
 * internal_name은 정규화되어 있어야 하며, ".."는 거부된다.
 * 부모 디렉터리들은 자동으로 생성한다.
 */
static char* vag_build_safe_output_path(const char* output_dir,
                                        const char* internal_name,
                                        vag_error* err)
{
    char* rel = vag_normalize_virtual_path(internal_name);

    if (!rel) {
        *err = VAG_ERR_PATH;
        return NULL;
    }

    if (rel[0] == '\0') {
        free(rel);
        *err = VAG_ERR_INVALID_NAME;
        return NULL;
    }

    const char* base = (output_dir && output_dir[0] != '\0') ? output_dir : ".";
    size_t base_len = strlen(base);
    size_t rel_len = strlen(rel);

    if (base_len > SIZE_MAX - rel_len - 2) {
        free(rel);
        *err = VAG_ERR_TOO_LARGE;
        return NULL;
    }

    int need_slash =
        (base_len > 0 && base[base_len - 1] != '/' && base[base_len - 1] != '\\') ? 1 : 0;

    size_t total_len = base_len + (need_slash ? 1u : 0u) + rel_len;

    char* full = (char*)malloc(total_len + 1);
    if (!full) {
        free(rel);
        *err = VAG_ERR_NO_MEMORY;
        return NULL;
    }

    size_t pos = 0;

    memcpy(full + pos, base, base_len);
    pos += base_len;

    if (need_slash) {
        full[pos++] = '/';
    }

    memcpy(full + pos, rel, rel_len + 1);
    free(rel);

    /* 경로 구분자를 '/'로 통일 */
    {
        char* t;
        for (t = full; *t != '\0'; ++t) {
            if (*t == '\\') {
                *t = '/';
            }
        }
    }

    /* 부모 디렉터리 생성 */
    char* last_slash = strrchr(full, '/');

    if (last_slash && last_slash != full) {
        *last_slash = '\0';

        if (vag_mkdirs(full) != 0) {
            free(full);
            *err = VAG_ERR_IO;
            return NULL;
        }

        *last_slash = '/';
    }

    *err = VAG_OK;
    return full;
}

/* ------------------------------------------------------------------ */
/* Internal: virtual tree                                              */
/* ------------------------------------------------------------------ */

struct vag_tree_node {
    char* name;

    int is_dir;

    /* SIZE_MAX면 파일 아님(디렉터리 전용) */
    size_t entry_index;

    vag_tree_node** children;
    size_t child_count;
    size_t child_capacity;
};

static int vag_tree_node_is_dir(const vag_tree_node* node)
{
    if (!node) {
        return 0;
    }
    return (node->is_dir || node->child_count > 0) ? 1 : 0;
}

static vag_tree_node* vag_tree_new_node_len(const char* s,
                                            size_t len,
                                            int is_dir,
                                            size_t entry_index)
{
    if (!s) {
        s = "";
        len = 0;
    }

    vag_tree_node* node = (vag_tree_node*)calloc(1, sizeof(vag_tree_node));
    if (!node) {
        return NULL;
    }

    node->name = (char*)malloc(len + 1);
    if (!node->name) {
        free(node);
        return NULL;
    }

    memcpy(node->name, s, len);
    node->name[len] = '\0';

    node->is_dir = is_dir;
    node->entry_index = entry_index;

    node->children = NULL;
    node->child_count = 0;
    node->child_capacity = 0;

    return node;
}

static void vag_tree_free_node(vag_tree_node* node)
{
    if (!node) {
        return;
    }

    size_t i;
    for (i = 0; i < node->child_count; ++i) {
        vag_tree_free_node(node->children[i]);
    }

    free(node->children);
    free(node->name);
    free(node);
}

static int vag_tree_reserve(vag_tree_node* node, size_t needed)
{
    if (needed <= node->child_capacity) {
        return 1;
    }

    size_t cap = (node->child_capacity != 0) ? node->child_capacity : 4;

    while (cap < needed) {
        if (cap > SIZE_MAX / 2) {
            cap = needed;
            break;
        }
        cap *= 2;
    }

    if (cap > SIZE_MAX / sizeof(vag_tree_node*)) {
        return 0;
    }

    vag_tree_node** tmp =
        (vag_tree_node**)realloc(node->children, cap * sizeof(vag_tree_node*));

    if (!tmp) {
        return 0;
    }

    node->children = tmp;
    node->child_capacity = cap;

    return 1;
}

static vag_tree_node* vag_tree_find_child(vag_tree_node* node,
                                          const char* comp,
                                          size_t len)
{
    if (!node || !comp) {
        return NULL;
    }

    size_t i;

    for (i = 0; i < node->child_count; ++i) {
        vag_tree_node* child = node->children[i];

        if (strlen(child->name) == len &&
            strncmp(child->name, comp, len) == 0) {
            return child;
        }
    }

    return NULL;
}

static vag_tree_node* vag_tree_add_child(vag_tree_node* parent,
                                         vag_tree_node* child)
{
    if (!parent || !child) {
        return NULL;
    }

    if (parent->child_count >= SIZE_MAX) {
        return NULL;
    }

    if (!vag_tree_reserve(parent, parent->child_count + 1)) {
        return NULL;
    }

    parent->children[parent->child_count++] = child;
    return child;
}

static vag_tree_node* vag_tree_get_or_add_dir(vag_tree_node* parent,
                                              const char* comp,
                                              size_t len)
{
    vag_tree_node* child = vag_tree_find_child(parent, comp, len);

    if (child) {
        child->is_dir = 1;
        return child;
    }

    child = vag_tree_new_node_len(comp, len, 1, SIZE_MAX);
    if (!child) {
        return NULL;
    }

    if (!vag_tree_add_child(parent, child)) {
        vag_tree_free_node(child);
        return NULL;
    }

    return child;
}

static int vag_tree_insert_entry(vag_archive* ar, size_t entry_index)
{
    if (!ar || !ar->tree_root || entry_index >= ar->count) {
        return 0;
    }

    char* norm = vag_normalize_virtual_path(ar->entries[entry_index].name);

    if (!norm) {
        return 0;
    }

    if (norm[0] == '\0') {
        free(norm);
        return 1;
    }

    vag_tree_node* cur = ar->tree_root;
    char* start = norm;
    int failed = 0;

    while (1) {
        char* slash = strchr(start, '/');

        size_t len = slash ? (size_t)(slash - start) : strlen(start);
        int is_last = (slash == NULL);

        if (len == 0) {
            if (is_last) {
                break;
            }
            start = slash + 1;
            continue;
        }

        if (is_last) {
            vag_tree_node* child = vag_tree_find_child(cur, start, len);

            if (!child) {
                child = vag_tree_new_node_len(start, len, 0, entry_index);

                if (!child) {
                    failed = 1;
                    break;
                }

                if (!vag_tree_add_child(cur, child)) {
                    vag_tree_free_node(child);
                    failed = 1;
                    break;
                }
            } else {
                child->entry_index = entry_index;
            }

            break;
        } else {
            vag_tree_node* dir = vag_tree_get_or_add_dir(cur, start, len);

            if (!dir) {
                failed = 1;
                break;
            }

            cur = dir;
            start = slash + 1;
        }
    }

    free(norm);

    return failed ? 0 : 1;
}

static int vag_tree_cmp(const void* a, const void* b)
{
    const vag_tree_node* na = *(const vag_tree_node* const*)a;
    const vag_tree_node* nb = *(const vag_tree_node* const*)b;

    int adir = vag_tree_node_is_dir(na);
    int bdir = vag_tree_node_is_dir(nb);

    if (adir != bdir) {
        return adir ? -1 : 1;
    }

    return strcmp(na->name ? na->name : "", nb->name ? nb->name : "");
}

static void vag_tree_sort_node(vag_tree_node* node)
{
    if (!node) {
        return;
    }

    if (node->child_count > 1) {
        qsort(node->children,
              node->child_count,
              sizeof(vag_tree_node*),
              vag_tree_cmp);
    }

    size_t i;
    for (i = 0; i < node->child_count; ++i) {
        vag_tree_sort_node(node->children[i]);
    }
}

static int vag_build_tree(vag_archive* ar)
{
    if (!ar) {
        return 0;
    }

    if (ar->tree_root) {
        return 1;
    }

    ar->tree_root = vag_tree_new_node_len("", 0, 1, SIZE_MAX);
    if (!ar->tree_root) {
        return 0;
    }

    size_t i;

    for (i = 0; i < ar->count; ++i) {
        if (!vag_tree_insert_entry(ar, i)) {
            vag_tree_free_node(ar->tree_root);
            ar->tree_root = NULL;
            return 0;
        }
    }

    vag_tree_sort_node(ar->tree_root);

    return 1;
}

static void vag_tree_print_node(const vag_archive* ar,
                                const vag_tree_node* node,
                                int depth)
{
    if (!node) {
        return;
    }

    size_t i;

    for (i = 0; i < node->child_count; ++i) {
        const vag_tree_node* child = node->children[i];

        int j;
        for (j = 0; j < depth; ++j) {
            printf("  ");
        }

        printf("%s", child->name);

        if (vag_tree_node_is_dir(child)) {
            printf("/");
        }

        if (child->entry_index != SIZE_MAX && child->entry_index < ar->count) {
            const vag_entry* e = &ar->entries[child->entry_index];
            printf("  [%llu bytes, crc32 %08X]",
                   (unsigned long long)e->size,
                   (unsigned)e->crc32);
        }

        printf("\n");

        if (child->child_count > 0) {
            vag_tree_print_node(ar, child, depth + 1);
        }
    }
}

/* ------------------------------------------------------------------ */
/* Internal: archive memory helpers                                    */
/* ------------------------------------------------------------------ */

static void vag_free_archive_contents(vag_archive* ar)
{
    if (!ar) {
        return;
    }

    if (ar->entries) {
        size_t i;
        for (i = 0; i < ar->count; ++i) {
            free(ar->entries[i].name);
        }

        free(ar->entries);
        ar->entries = NULL;
    }

    ar->count = 0;
    ar->capacity = 0;

    if (ar->tree_root) {
        vag_tree_free_node(ar->tree_root);
        ar->tree_root = NULL;
    }
}

static void vag_free_archive(vag_archive* ar)
{
    if (!ar) {
        return;
    }

    if (ar->fp) {
        fclose(ar->fp);
        ar->fp = NULL;
    }

    vag_free_archive_contents(ar);
    free(ar);
}

/* ------------------------------------------------------------------ */
/* Internal: entry array helpers                                       */
/* ------------------------------------------------------------------ */

static int vag_reserve(vag_archive* ar, size_t needed)
{
    if (needed <= ar->capacity) {
        return 1;
    }

    size_t cap = (ar->capacity != 0) ? ar->capacity : 8;

    while (cap < needed) {
        if (cap > SIZE_MAX / 2) {
            cap = needed;
            break;
        }
        cap *= 2;
    }

    if (cap > SIZE_MAX / sizeof(vag_entry)) {
        return 0;
    }

    vag_entry* tmp = (vag_entry*)realloc(ar->entries, cap * sizeof(vag_entry));
    if (!tmp) {
        return 0;
    }

    ar->entries = tmp;
    ar->capacity = cap;

    return 1;
}

static const vag_entry* vag_find_entry(const vag_archive* ar, const char* name)
{
    size_t i;

    if (!ar || !name) {
        return NULL;
    }

    for (i = 0; i < ar->count; ++i) {
        if (strcmp(ar->entries[i].name, name) == 0) {
            return &ar->entries[i];
        }
    }

    return NULL;
}

/* ------------------------------------------------------------------ */
/* Internal: central directory writer                                  */
/* ------------------------------------------------------------------ */

static vag_error vag_write_central_directory(vag_archive* ar)
{
    uint8_t tmp[32];
    size_t i;
    vag_error err;

    if (ar->count > UINT32_MAX) {
        return VAG_ERR_TOO_MANY;
    }

    if (vag_seek_set(ar->fp, ar->next_data_offset) != 0) {
        return VAG_ERR_IO;
    }

    for (i = 0; i < ar->count; ++i) {
        size_t len = strlen(ar->entries[i].name);

        if (len == 0) {
            return VAG_ERR_INVALID_NAME;
        }

        if (len > (size_t)VAG_MAX_NAME_LEN) {
            return VAG_ERR_NAME_TOO_LONG;
        }

        /* name_len */
        vag_put_u16(tmp, (uint16_t)len);
        err = vag_write_exact(ar->fp, tmp, 2);
        if (err != VAG_OK) {
            return err;
        }

        /* name */
        err = vag_write_exact(ar->fp, ar->entries[i].name, len);
        if (err != VAG_OK) {
            return err;
        }

        /* offset, size, mtime, crc32 */
        vag_put_u64(tmp, ar->entries[i].offset);
        vag_put_u64(tmp + 8, ar->entries[i].size);
        vag_put_u64(tmp + 16, ar->entries[i].mtime);
        vag_put_u32(tmp + 24, ar->entries[i].crc32);

        err = vag_write_exact(ar->fp, tmp, 28);
        if (err != VAG_OK) {
            return err;
        }
    }

    /* Footer */
    vag_put_u64(tmp, ar->next_data_offset);
    vag_put_u32(tmp + 8, (uint32_t)ar->count);
    vag_put_u32(tmp + 12, VAG_VERSION);
    memcpy(tmp + 16, VAG_MAGIC_TEXT, VAG_MAGIC_LEN);

    err = vag_write_exact(ar->fp, tmp, VAG_FOOTER_SIZE);
    if (err != VAG_OK) {
        return err;
    }

    if (fflush(ar->fp) != 0) {
        return VAG_ERR_IO;
    }

    if (ferror(ar->fp)) {
        return VAG_ERR_IO;
    }

    return VAG_OK;
}

/* ------------------------------------------------------------------ */
/* Internal: extract one entry                                         */
/* ------------------------------------------------------------------ */

static vag_error vag_extract_entry_to_path(const vag_archive* ar,
                                           const vag_entry* entry,
                                           const char* output_path,
                                           unsigned flags)
{
    if (!ar || !entry || !output_path) {
        return VAG_ERR_NULL;
    }

    if (ar->is_writing) {
        return VAG_ERR_INVALID_MODE;
    }

    if (vag_seek_set(ar->fp, entry->offset) != 0) {
        return VAG_ERR_IO;
    }

    FILE* out = fopen(output_path, "wb");
    if (!out) {
        return VAG_ERR_OPEN_WRITE;
    }

    int verify = ((flags & VAG_EXTRACT_FLAG_NO_VERIFY) == 0);

    vag_error err = VAG_OK;
    uint64_t remaining = entry->size;
    uint32_t crc = 0;

    if (remaining > 0) {
        unsigned char* buf = (unsigned char*)malloc(VAG_COPY_BUF_SIZE);

        if (!buf) {
            err = VAG_ERR_NO_MEMORY;
        }

        while (err == VAG_OK && remaining > 0) {
            size_t want = (remaining > (uint64_t)VAG_COPY_BUF_SIZE)
                              ? (size_t)VAG_COPY_BUF_SIZE
                              : (size_t)remaining;

            size_t r = fread(buf, 1, want, ar->fp);

            if (r != want) {
                err = ferror(ar->fp) ? VAG_ERR_IO : VAG_ERR_CORRUPT;
                break;
            }

            if (verify) {
                crc = vag_crc32_update(crc, buf, r);
            }

            if (fwrite(buf, 1, r, out) != r) {
                err = VAG_ERR_IO;
                break;
            }

            remaining -= (uint64_t)r;
        }

        free(buf);
    }

    if (err == VAG_OK && fflush(out) != 0) {
        err = VAG_ERR_IO;
    }

    if (fclose(out) != 0 && err == VAG_OK) {
        err = VAG_ERR_IO;
    }

    if (err != VAG_OK) {
        remove(output_path);
        return err;
    }

    if (verify && crc != entry->crc32) {
        remove(output_path);
        return VAG_ERR_CHECKSUM;
    }

    return VAG_OK;
}

/* ------------------------------------------------------------------ */
/* Internal: partial extract helpers                                   */
/* ------------------------------------------------------------------ */

static int vag_entry_matches_norm(const char* entry_name,
                                  const char* pattern_norm)
{
    if (!entry_name) {
        return 0;
    }

    if (!pattern_norm || pattern_norm[0] == '\0') {
        return 1;
    }

    char* norm = vag_normalize_virtual_path(entry_name);
    if (!norm) {
        return 0;
    }

    int match = 0;
    size_t plen = strlen(pattern_norm);

    if (strcmp(norm, pattern_norm) == 0) {
        match = 1;
    } else if (strncmp(norm, pattern_norm, plen) == 0 && norm[plen] == '/') {
        match = 1;
    }

    free(norm);

    return match;
}

/* ------------------------------------------------------------------ */
/* Public implementation                                               */
/* ------------------------------------------------------------------ */

const char* vag_strerror(vag_error err)
{
    switch (err) {
        case VAG_OK:
            return "성공";
        case VAG_ERR_NULL:
            return "NULL 인자가 전달됨";
        case VAG_ERR_INVALID_NAME:
            return "잘못된 파일 이름(빈 이름 등)";
        case VAG_ERR_OPEN_READ:
            return "파일을 읽기 모드로 열기 실패";
        case VAG_ERR_OPEN_WRITE:
            return "파일을 쓰기 모드로 열기 실패";
        case VAG_ERR_IO:
            return "입출력 오류";
        case VAG_ERR_NO_MEMORY:
            return "메모리 부족";
        case VAG_ERR_NOT_VAG:
            return "VAG 파일이 아님";
        case VAG_ERR_CORRUPT:
            return "손상된 VAG 파일";
        case VAG_ERR_NOT_FOUND:
            return "요청한 파일을 찾지 못함";
        case VAG_ERR_NAME_TOO_LONG:
            return "파일 이름이 너무 김";
        case VAG_ERR_DUPLICATE:
            return "중복된 파일 이름";
        case VAG_ERR_INVALID_MODE:
            return "현재 모드에서 허용되지 않는 작업";
        case VAG_ERR_TOO_LARGE:
            return "파일/오프셋이 너무 큼";
        case VAG_ERR_TOO_MANY:
            return "항목 수가 너무 많음";
        case VAG_ERR_UNSUPPORTED:
            return "지원하지 않는 VAG 버전";
        case VAG_ERR_CHECKSUM:
            return "체크섬(CRC32) 불일치";
        case VAG_ERR_PATH:
            return "잘못된 경로(경로 순회 시도 또는 정규화 실패)";
        default:
            return "알 수 없는 오류";
    }
}

vag_archive* vag_open_write(const char* archive_path, vag_error* err_out)
{
    vag_error err = VAG_OK;
    vag_archive* ar = NULL;
    FILE* fp = NULL;

    if (!archive_path) {
        err = VAG_ERR_NULL;
        goto done;
    }

    fp = fopen(archive_path, "wb");
    if (!fp) {
        err = VAG_ERR_OPEN_WRITE;
        goto done;
    }

    ar = (vag_archive*)calloc(1, sizeof(vag_archive));
    if (!ar) {
        err = VAG_ERR_NO_MEMORY;
        goto fail;
    }

    ar->fp = fp;
    ar->is_writing = 1;
    ar->broken = 0;
    ar->last_error = VAG_OK;
    ar->entries = NULL;
    ar->count = 0;
    ar->capacity = 0;
    ar->next_data_offset = 0;
    ar->tree_root = NULL;

    if (err_out) {
        *err_out = VAG_OK;
    }

    return ar;

fail:
    if (fp) {
        fclose(fp);
    }

done:
    if (err_out) {
        *err_out = err;
    }

    return NULL;
}

vag_error vag_add_file(vag_archive* ar,
                       const char* archive_name,
                       const char* disk_path)
{
    if (!ar || !archive_name || !disk_path) {
        return VAG_ERR_NULL;
    }

    if (!ar->is_writing) {
        return VAG_ERR_INVALID_MODE;
    }

    if (ar->broken) {
        return (ar->last_error != VAG_OK) ? ar->last_error : VAG_ERR_IO;
    }

    /* 이름을 가상 경로로 정규화한다. */
    char* norm_name = vag_normalize_virtual_path(archive_name);

    if (!norm_name) {
        return VAG_ERR_PATH;
    }

    if (norm_name[0] == '\0') {
        free(norm_name);
        return VAG_ERR_INVALID_NAME;
    }

    size_t name_len = strlen(norm_name);

    if (name_len > (size_t)VAG_MAX_NAME_LEN) {
        free(norm_name);
        return VAG_ERR_NAME_TOO_LONG;
    }

    if (vag_find_entry(ar, norm_name) != NULL) {
        free(norm_name);
        return VAG_ERR_DUPLICATE;
    }

    if (ar->count >= (size_t)UINT32_MAX) {
        free(norm_name);
        return VAG_ERR_TOO_MANY;
    }

    FILE* in = fopen(disk_path, "rb");
    if (!in) {
        free(norm_name);
        return VAG_ERR_OPEN_READ;
    }

    unsigned char* buf = (unsigned char*)malloc(VAG_COPY_BUF_SIZE);
    if (!buf) {
        fclose(in);
        free(norm_name);
        return VAG_ERR_NO_MEMORY;
    }

    uint64_t start_offset = ar->next_data_offset;
    uint64_t file_size = 0;
    uint32_t crc = 0;

    vag_error err = VAG_OK;

    while (1) {
        size_t r = fread(buf, 1, VAG_COPY_BUF_SIZE, in);

        if (r == 0) {
            if (ferror(in)) {
                err = VAG_ERR_IO;
            }
            break;
        }

        if (file_size > UINT64_MAX - (uint64_t)r) {
            err = VAG_ERR_TOO_LARGE;
            break;
        }

        crc = vag_crc32_update(crc, buf, r);

        if (fwrite(buf, 1, r, ar->fp) != r) {
            err = VAG_ERR_IO;
            break;
        }

        file_size += (uint64_t)r;
    }

    free(buf);
    fclose(in);

    if (err != VAG_OK) {
        free(norm_name);
        ar->broken = 1;
        ar->last_error = err;
        return err;
    }

    if (start_offset > UINT64_MAX - file_size) {
        free(norm_name);
        ar->broken = 1;
        ar->last_error = VAG_ERR_TOO_LARGE;
        return VAG_ERR_TOO_LARGE;
    }

    if (!vag_reserve(ar, ar->count + 1)) {
        free(norm_name);
        ar->broken = 1;
        ar->last_error = VAG_ERR_NO_MEMORY;
        return VAG_ERR_NO_MEMORY;
    }

    uint64_t mtime = vag_get_file_mtime(disk_path);

    ar->entries[ar->count].name = norm_name;
    ar->entries[ar->count].offset = start_offset;
    ar->entries[ar->count].size = file_size;
    ar->entries[ar->count].mtime = mtime;
    ar->entries[ar->count].crc32 = crc;
    ar->count++;

    ar->next_data_offset = start_offset + file_size;

    /* entries가 변경되었으므로 기존 tree는 무효화 */
    if (ar->tree_root) {
        vag_tree_free_node(ar->tree_root);
        ar->tree_root = NULL;
    }

    return VAG_OK;
}

vag_error vag_close(vag_archive* ar)
{
    if (!ar) {
        return VAG_OK;
    }

    vag_error err = ar->last_error;

    if (ar->is_writing && !ar->broken) {
        vag_error cd_err = vag_write_central_directory(ar);

        if (cd_err != VAG_OK) {
            err = cd_err;
            ar->broken = 1;
        }
    }

    if (ar->broken && err == VAG_OK) {
        err = VAG_ERR_IO;
    }

    if (ar->fp) {
        if (fclose(ar->fp) != 0 && err == VAG_OK) {
            err = VAG_ERR_IO;
        }
        ar->fp = NULL;
    }

    vag_free_archive_contents(ar);
    free(ar);

    return err;
}

void vag_abort(vag_archive* ar)
{
    vag_free_archive(ar);
}

vag_archive* vag_open_read(const char* archive_path, vag_error* err_out)
{
    vag_error err = VAG_OK;
    vag_archive* ar = NULL;
    FILE* fp = NULL;

    if (!archive_path) {
        err = VAG_ERR_NULL;
        goto done;
    }

    fp = fopen(archive_path, "rb");
    if (!fp) {
        err = VAG_ERR_OPEN_READ;
        goto done;
    }

    ar = (vag_archive*)calloc(1, sizeof(vag_archive));
    if (!ar) {
        err = VAG_ERR_NO_MEMORY;
        goto fail;
    }

    ar->fp = fp;
    ar->is_writing = 0;
    ar->broken = 0;
    ar->last_error = VAG_OK;
    ar->tree_root = NULL;

    if (fseek(fp, 0L, SEEK_END) != 0) {
        err = VAG_ERR_IO;
        goto fail;
    }

    uint64_t file_size = vag_tell_pos(fp);

    if (file_size == UINT64_MAX) {
        err = VAG_ERR_IO;
        goto fail;
    }

    if (file_size < VAG_FOOTER_SIZE) {
        err = VAG_ERR_NOT_VAG;
        goto fail;
    }

    if (vag_seek_set(fp, file_size - VAG_FOOTER_SIZE) != 0) {
        err = VAG_ERR_IO;
        goto fail;
    }

    uint8_t footer[VAG_FOOTER_SIZE];

    err = vag_read_exact(fp, footer, sizeof(footer));
    if (err != VAG_OK) {
        goto fail;
    }

    if (memcmp(footer + 16, VAG_MAGIC_TEXT, VAG_MAGIC_LEN) != 0) {
        err = VAG_ERR_NOT_VAG;
        goto fail;
    }

    uint64_t cd_offset = vag_get_u64(footer);
    uint32_t expected_count = vag_get_u32(footer + 8);
    uint32_t version = vag_get_u32(footer + 12);

    if (version != VAG_VERSION) {
        err = VAG_ERR_UNSUPPORTED;
        goto fail;
    }

    uint64_t cd_end = file_size - VAG_FOOTER_SIZE;

    if (cd_offset > cd_end) {
        err = VAG_ERR_CORRUPT;
        goto fail;
    }

    uint64_t cd_size = cd_end - cd_offset;

    /* v2 minimum entry size: name_len(2) + offset(8) + size(8) + mtime(8) + crc(4) = 30 */
    if (expected_count > 0) {
        if ((uint64_t)expected_count > cd_size / 30u) {
            err = VAG_ERR_CORRUPT;
            goto fail;
        }

        if ((uint64_t)expected_count > (uint64_t)(SIZE_MAX / sizeof(vag_entry))) {
            err = VAG_ERR_NO_MEMORY;
            goto fail;
        }

        ar->entries = (vag_entry*)malloc((size_t)expected_count * sizeof(vag_entry));
        if (!ar->entries) {
            err = VAG_ERR_NO_MEMORY;
            goto fail;
        }

        ar->capacity = (size_t)expected_count;
    }

    if (vag_seek_set(fp, cd_offset) != 0) {
        err = VAG_ERR_CORRUPT;
        goto fail;
    }

    uint64_t pos = cd_offset;
    ar->count = 0;

    uint32_t i;

    for (i = 0; i < expected_count; ++i) {
        if (pos + 2u > cd_end) {
            err = VAG_ERR_CORRUPT;
            goto fail;
        }

        uint8_t lenbuf[2];

        err = vag_read_exact(fp, lenbuf, sizeof(lenbuf));
        if (err != VAG_OK) {
            goto fail;
        }

        uint16_t name_len = vag_get_u16(lenbuf);
        pos += 2u;

        if (name_len == 0) {
            err = VAG_ERR_CORRUPT;
            goto fail;
        }

        uint64_t need = (uint64_t)name_len + 28u;

        if (need > cd_end - pos) {
            err = VAG_ERR_CORRUPT;
            goto fail;
        }

        char* name = (char*)malloc((size_t)name_len + 1u);
        if (!name) {
            err = VAG_ERR_NO_MEMORY;
            goto fail;
        }

        err = vag_read_exact(fp, name, (size_t)name_len);
        if (err != VAG_OK) {
            free(name);
            goto fail;
        }

        if (memchr(name, 0, (size_t)name_len) != NULL) {
            free(name);
            err = VAG_ERR_CORRUPT;
            goto fail;
        }

        name[name_len] = '\0';

        /* 읽은 이름을 정규화하여 보안/일관성 확보 */
        char* norm = vag_normalize_virtual_path(name);
        free(name);

        if (!norm) {
            err = VAG_ERR_CORRUPT;
            goto fail;
        }

        if (norm[0] == '\0') {
            free(norm);
            err = VAG_ERR_CORRUPT;
            goto fail;
        }

        if (strlen(norm) > (size_t)VAG_MAX_NAME_LEN) {
            free(norm);
            err = VAG_ERR_CORRUPT;
            goto fail;
        }

        if (vag_find_entry(ar, norm) != NULL) {
            free(norm);
            err = VAG_ERR_CORRUPT;
            goto fail;
        }

        uint8_t fields[28];

        err = vag_read_exact(fp, fields, sizeof(fields));
        if (err != VAG_OK) {
            free(norm);
            goto fail;
        }

        uint64_t off = vag_get_u64(fields);
        uint64_t sz = vag_get_u64(fields + 8);
        uint64_t mtime = vag_get_u64(fields + 16);
        uint32_t crc = vag_get_u32(fields + 24);

        /* 데이터는 central directory 앞에 있어야 한다. */
        if (off > cd_offset || sz > cd_offset - off) {
            free(norm);
            err = VAG_ERR_CORRUPT;
            goto fail;
        }

        ar->entries[ar->count].name = norm;
        ar->entries[ar->count].offset = off;
        ar->entries[ar->count].size = sz;
        ar->entries[ar->count].mtime = mtime;
        ar->entries[ar->count].crc32 = crc;
        ar->count++;

        pos += need;
    }

    if (err_out) {
        *err_out = VAG_OK;
    }

    return ar;

fail:
    if (ar) {
        vag_free_archive(ar);
    } else if (fp) {
        fclose(fp);
    }

done:
    if (err_out) {
        *err_out = err;
    }

    return NULL;
}

vag_error vag_create(const char* archive_path,
                     const char* const* file_paths,
                     const char* const* archive_names,
                     size_t file_count)
{
    if (!archive_path) {
        return VAG_ERR_NULL;
    }

    if (file_count > 0 && !file_paths) {
        return VAG_ERR_NULL;
    }

    vag_error err = VAG_OK;

    vag_archive* ar = vag_open_write(archive_path, &err);
    if (!ar) {
        return err;
    }

    size_t i;

    for (i = 0; i < file_count; ++i) {
        const char* disk_path = file_paths[i];

        if (!disk_path) {
            err = VAG_ERR_NULL;
            break;
        }

        const char* archive_name = NULL;

        if (archive_names) {
            archive_name = archive_names[i];

            if (!archive_name) {
                err = VAG_ERR_NULL;
                break;
            }
        } else {
            /*
             * archive_names가 NULL이면 disk_path를 가상 경로로 사용한다.
             * vag_add_file에서 정규화된다.
             */
            archive_name = disk_path;
        }

        err = vag_add_file(ar, archive_name, disk_path);

        if (err != VAG_OK) {
            break;
        }
    }

    if (err != VAG_OK) {
        vag_abort(ar);
        remove(archive_path);
        return err;
    }

    err = vag_close(ar);

    if (err != VAG_OK) {
        remove(archive_path);
    }

    return err;
}

vag_error vag_extract(const char* archive_path,
                      const char* archive_name,
                      const char* output_path)
{
    if (!archive_path || !archive_name || !output_path) {
        return VAG_ERR_NULL;
    }

    vag_error err = VAG_OK;

    vag_archive* ar = vag_open_read(archive_path, &err);
    if (!ar) {
        return err;
    }

    const vag_entry* entry = vag_find(ar, archive_name);

    if (!entry) {
        err = VAG_ERR_NOT_FOUND;
    } else {
        err = vag_extract_entry_to_path(ar, entry, output_path, VAG_EXTRACT_FLAG_NONE);
    }

    vag_error close_err = vag_close(ar);

    if (err == VAG_OK) {
        err = close_err;
    }

    return err;
}

vag_error vag_extract_partial(vag_archive* ar,
                              const char* output_dir,
                              const char* const* patterns,
                              size_t pattern_count,
                              unsigned flags)
{
    if (!ar) {
        return VAG_ERR_NULL;
    }

    if (ar->is_writing) {
        return VAG_ERR_INVALID_MODE;
    }

    if (pattern_count > 0 && !patterns) {
        return VAG_ERR_NULL;
    }

    vag_error err = VAG_OK;

    unsigned char* done =
        (unsigned char*)calloc(ar->count ? ar->count : 1, 1);

    if (!done) {
        return VAG_ERR_NO_MEMORY;
    }

    char** norm_patterns = NULL;
    int extract_all = (pattern_count == 0);
    size_t extracted = 0;

    if (pattern_count > 0) {
        norm_patterns = (char**)calloc(pattern_count, sizeof(char*));

        if (!norm_patterns) {
            err = VAG_ERR_NO_MEMORY;
            goto cleanup;
        }

        size_t p;

        for (p = 0; p < pattern_count; ++p) {
            if (!patterns[p]) {
                err = VAG_ERR_NULL;
                goto cleanup;
            }

            norm_patterns[p] = vag_normalize_virtual_path(patterns[p]);

            if (!norm_patterns[p]) {
                err = VAG_ERR_PATH;
                goto cleanup;
            }

            if (norm_patterns[p][0] == '\0') {
                extract_all = 1;
            }
        }
    }

    if (extract_all) {
        size_t i;

        for (i = 0; i < ar->count && err == VAG_OK; ++i) {
            if (done[i]) {
                continue;
            }

            char* out_path =
                vag_build_safe_output_path(output_dir, ar->entries[i].name, &err);

            if (!out_path) {
                break;
            }

            err = vag_extract_entry_to_path(ar,
                                            &ar->entries[i],
                                            out_path,
                                            flags);

            free(out_path);

            if (err == VAG_OK) {
                done[i] = 1;
                extracted++;
            }
        }
    } else {
        size_t p, i;

        for (p = 0; p < pattern_count && err == VAG_OK; ++p) {
            for (i = 0; i < ar->count && err == VAG_OK; ++i) {
                if (done[i]) {
                    continue;
                }

                if (!vag_entry_matches_norm(ar->entries[i].name, norm_patterns[p])) {
                    continue;
                }

                char* out_path =
                    vag_build_safe_output_path(output_dir, ar->entries[i].name, &err);

                if (!out_path) {
                    break;
                }

                err = vag_extract_entry_to_path(ar,
                                                &ar->entries[i],
                                                out_path,
                                                flags);

                free(out_path);

                if (err == VAG_OK) {
                    done[i] = 1;
                    extracted++;
                }
            }
        }
    }

    if (err == VAG_OK && !extract_all && extracted == 0) {
        err = VAG_ERR_NOT_FOUND;
    }

cleanup:
    if (norm_patterns) {
        size_t p;

        for (p = 0; p < pattern_count; ++p) {
            free(norm_patterns[p]);
        }

        free(norm_patterns);
    }

    free(done);

    return err;
}

vag_error vag_extract_partial_from_file(const char* archive_path,
                                        const char* output_dir,
                                        const char* const* patterns,
                                        size_t pattern_count,
                                        unsigned flags)
{
    if (!archive_path) {
        return VAG_ERR_NULL;
    }

    vag_error err = VAG_OK;

    vag_archive* ar = vag_open_read(archive_path, &err);
    if (!ar) {
        return err;
    }

    err = vag_extract_partial(ar, output_dir, patterns, pattern_count, flags);

    vag_error close_err = vag_close(ar);

    if (err == VAG_OK) {
        err = close_err;
    }

    return err;
}

vag_error vag_extract_all(const char* archive_path,
                          const char* output_dir,
                          unsigned flags)
{
    return vag_extract_partial_from_file(archive_path,
                                         output_dir,
                                         NULL,
                                         0,
                                         flags);
}

size_t vag_count(const vag_archive* ar)
{
    return ar ? ar->count : 0;
}

const vag_entry* vag_get(const vag_archive* ar, size_t index)
{
    if (!ar || index >= ar->count) {
        return NULL;
    }
    return &ar->entries[index];
}

const vag_entry* vag_find(const vag_archive* ar, const char* archive_name)
{
    if (!ar || !archive_name) {
        return NULL;
    }

    const vag_entry* exact = vag_find_entry(ar, archive_name);
    if (exact) {
        return exact;
    }

    char* norm = vag_normalize_virtual_path(archive_name);
    if (!norm) {
        return NULL;
    }

    const vag_entry* found = vag_find_entry(ar, norm);
    free(norm);

    return found;
}

/* ------------------------------------------------------------------ */
/* Public: tree API                                                    */
/* ------------------------------------------------------------------ */

const vag_tree_node* vag_tree_root(vag_archive* ar)
{
    if (!ar) {
        return NULL;
    }

    if (!ar->tree_root) {
        if (!vag_build_tree(ar)) {
            return NULL;
        }
    }

    return ar->tree_root;
}

const vag_tree_node* vag_tree_find_node(vag_archive* ar, const char* path)
{
    const vag_tree_node* root = vag_tree_root(ar);

    if (!root) {
        return NULL;
    }

    if (!path) {
        return root;
    }

    char* norm = vag_normalize_virtual_path(path);

    if (!norm) {
        return NULL;
    }

    if (norm[0] == '\0') {
        free(norm);
        return root;
    }

    const vag_tree_node* cur = root;
    char* start = norm;

    while (cur && *start != '\0') {
        char* slash = strchr(start, '/');

        size_t len = slash ? (size_t)(slash - start) : strlen(start);

        vag_tree_node* child =
            vag_tree_find_child((vag_tree_node*)cur, start, len);

        if (!child) {
            cur = NULL;
            break;
        }

        cur = child;

        if (!slash) {
            break;
        }

        start = slash + 1;
    }

    free(norm);

    return cur;
}

size_t vag_tree_child_count(const vag_tree_node* node)
{
    return node ? node->child_count : 0;
}

const vag_tree_node* vag_tree_child(const vag_tree_node* node, size_t index)
{
    if (!node || index >= node->child_count) {
        return NULL;
    }
    return node->children[index];
}

const char* vag_tree_name(const vag_tree_node* node)
{
    return node ? node->name : NULL;
}

int vag_tree_is_dir(const vag_tree_node* node)
{
    return vag_tree_node_is_dir(node);
}

size_t vag_tree_entry_index(const vag_tree_node* node)
{
    return node ? node->entry_index : SIZE_MAX;
}

void vag_tree_print(vag_archive* ar)
{
    if (!ar) {
        return;
    }

    const vag_tree_node* root = vag_tree_root(ar);

    if (!root) {
        return;
    }

    printf("/\n");
    vag_tree_print_node(ar, root, 0);
}

#ifdef __cplusplus
} /* extern "C" */
#endif

/* ------------------------------------------------------------------ */
/* Optional CLI                                                        */
/*                                                                     */
/* 빌드 예:                                                           */
/*   gcc -std=c99 -DVAG_MAIN vag.c -o vag                             */
/*                                                                     */
/* 사용 예:                                                           */
/*   vag create out.vag file1.txt dir/file2.txt                       */
/*   vag list out.vag                                                 */
/*   vag tree out.vag                                                 */
/*   vag extract out.vag dir/file2.txt ./file2.txt                    */
/*   vag extract-tree out.vag ./out dir                               */
/*   vag extract-tree out.vag ./out                                   */
/* ------------------------------------------------------------------ */

#ifdef VAG_MAIN

static void vag_print_usage(const char* prog)
{
    fprintf(stderr, "Usage:\n");
    fprintf(stderr, "  %s create <archive.vag> <file1> [file2 ...]\n", prog);
    fprintf(stderr, "  %s list <archive.vag>\n", prog);
    fprintf(stderr, "  %s tree <archive.vag>\n", prog);
    fprintf(stderr, "  %s extract <archive.vag> <internal_name> <output_path>\n", prog);
    fprintf(stderr, "  %s extract-tree <archive.vag> <output_dir> [pattern ...]\n", prog);
}

static int vag_cli_list(const char* archive_path)
{
    vag_error err = VAG_OK;

    vag_archive* ar = vag_open_read(archive_path, &err);
    if (!ar) {
        fprintf(stderr, "vag error: %s\n", vag_strerror(err));
        return 1;
    }

    size_t n = vag_count(ar);

    printf("VAG archive: %s (%zu entries)\n", archive_path, n);

    size_t i;

    for (i = 0; i < n; ++i) {
        const vag_entry* e = vag_get(ar, i);

        if (!e) {
            continue;
        }

        printf("%10llu bytes  crc32 %08X  mtime %llu  %s\n",
               (unsigned long long)e->size,
               (unsigned)e->crc32,
               (unsigned long long)e->mtime,
               e->name);
    }

    vag_close(ar);

    return 0;
}

static int vag_cli_tree(const char* archive_path)
{
    vag_error err = VAG_OK;

    vag_archive* ar = vag_open_read(archive_path, &err);
    if (!ar) {
        fprintf(stderr, "vag error: %s\n", vag_strerror(err));
        return 1;
    }

    if (!vag_tree_root(ar)) {
        fprintf(stderr, "vag error: tree build failed\n");
        vag_close(ar);
        return 1;
    }

    vag_tree_print(ar);

    vag_close(ar);

    return 0;
}

int main(int argc, char** argv)
{
    if (argc < 3) {
        vag_print_usage(argv[0]);
        return 1;
    }

    vag_error err = VAG_OK;

    if (strcmp(argv[1], "create") == 0) {
        if (argc < 4) {
            vag_print_usage(argv[0]);
            return 1;
        }

        size_t n = (size_t)(argc - 3);

        const char** files = (const char**)malloc(n * sizeof(const char*));
        if (!files) {
            fprintf(stderr, "vag error: %s\n", vag_strerror(VAG_ERR_NO_MEMORY));
            return 1;
        }

        size_t i;
        for (i = 0; i < n; ++i) {
            files[i] = argv[3 + i];
        }

        err = vag_create(argv[2], (const char* const*)files, NULL, n);

        free(files);

    } else if (strcmp(argv[1], "list") == 0) {
        return vag_cli_list(argv[2]);

    } else if (strcmp(argv[1], "tree") == 0) {
        return vag_cli_tree(argv[2]);

    } else if (strcmp(argv[1], "extract") == 0) {
        if (argc != 5) {
            vag_print_usage(argv[0]);
            return 1;
        }

        err = vag_extract(argv[2], argv[3], argv[4]);

    } else if (strcmp(argv[1], "extract-tree") == 0) {
        if (argc < 4) {
            vag_print_usage(argv[0]);
            return 1;
        }

        size_t pattern_count = (argc > 4) ? (size_t)(argc - 4) : 0;
        const char** patterns = NULL;

        if (pattern_count > 0) {
            patterns = (const char**)malloc(pattern_count * sizeof(const char*));

            if (!patterns) {
                fprintf(stderr, "vag error: %s\n", vag_strerror(VAG_ERR_NO_MEMORY));
                return 1;
            }

            size_t i;
            for (i = 0; i < pattern_count; ++i) {
                patterns[i] = argv[4 + i];
            }
        }

        err = vag_extract_partial_from_file(argv[2],
                                            argv[3],
                                            (const char* const*)patterns,
                                            pattern_count,
                                            VAG_EXTRACT_FLAG_NONE);

        free(patterns);

    } else {
        vag_print_usage(argv[0]);
        return 1;
    }

    if (err != VAG_OK) {
        fprintf(stderr, "vag error: %s\n", vag_strerror(err));
        return 1;
    }

    return 0;
}

#endif /* VAG_MAIN */
