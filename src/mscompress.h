#include <argp.h>
#include <zstd.h>

#define VERSION "0.0.1"
#define STATUS "Development"
#define ADDRESS "chrisagrams@gmail.com"

#define FORMAT_VERSION_MAJOR 1
#define FORMAT_VERSION_MINOR 0

#define BUFSIZE 4096

#define MAGIC_TAG 0x035F51B5
#define MESSAGE "MS Compress Format 1.0 Gao Laboratory at UIC"

struct arguments
{
  char *args[0];            /* ARG1 and ARG2 */
  char *in_file;            /* Argument for -i */
  char *out_file;           /* Argument for -o */
  int version;              /* The -v flag */
};

static error_t parse_opt (int key, char *arg, struct argp_state *state);

typedef struct
{
    int mz_fmt;
    int int_fmt;
    int compression;
    int total_spec;
    int populated;
} data_format_t;

typedef struct
{
    int* start_positions;
    int* end_positions;
    int* positions_len;
    int total_spec;
    size_t file_end;
} data_positions_t;


typedef struct 
{
    char* mem;
    size_t size;
    size_t max_size;
} data_block_t;

typedef struct
{
    char* mem;
    size_t size;
    struct cmp_block_t* next;
} cmp_block_t;

typedef struct 
{
    cmp_block_t* head;
    cmp_block_t* tail;

    int populated;
} cmp_blk_queue_t;


#define _32i_ 1000519
#define _16e_ 1000520
#define _32f_ 1000521
#define _64i_ 1000522
#define _64d_ 1000523

#define _zlib_ 1000574
#define _no_comp_ 1000576

#define _intensity_ 1000515
#define _mass_ 1000514


/* file.c */
void* get_mapping(int fd);
int remove_mapping(void* addr, int fd);
int get_blksize(char* path);
size_t get_filesize(char* path);
size_t write_to_file(int fd, char* buff, size_t n);
void write_header(int fd, char* compression_method, char* md5);

/* preproccess.c */
data_format_t* pattern_detect(char* input_map);
data_positions_t*find_binary(char* input_map, data_format_t* df);
void get_encoded_lengths(char* input_map, data_positions_t* dp);
data_positions_t** get_binary_divisions(data_positions_t* dp, int* blocksize, int* divisions, int threads);
data_positions_t** get_xml_divisions(data_positions_t* dp, data_positions_t** binary_divisions, int divisions);
void free_ddp(data_positions_t** ddp, int divisions);

/* sys.c */
int get_cpu_count();
int get_thread_id();


/* decode.c */
double* decode_binary(char* input_map, int start_position, int end_position, int compression_method, size_t* out_len);

/* compress.c */
typedef struct {
    char* input_map;
    data_positions_t* dp;
    data_format_t* df;
    size_t cmp_blk_size;

    cmp_blk_queue_t* ret;
} compress_args_t;
    
ZSTD_CCtx* alloc_cctx();
void * zstd_compress(ZSTD_CCtx* cctx, void* src_buff, size_t src_len, size_t* out_len, int compression_level);
void compress_routine(void* args);
void compress_parallel(char* input_map, data_positions_t** ddp, data_format_t* df, size_t cmp_blk_size, int divisions, int fd);

/* decompress.c */
ZSTD_DCtx* alloc_dctx();
void * zstd_decompress(ZSTD_DCtx* dctx, void* src_buff, size_t src_len, size_t org_len);