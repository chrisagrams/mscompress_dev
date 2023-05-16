#include <assert.h>
#include <float.h>
#include <math.h>
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <zstd.h>
#include "mscompress.h"

/* 
    @section Decoding functions
*/

void
algo_decode_lossless (void* args)
/**
 * @brief Lossless decoding function.
 * 
 * @param args Pointer to algo_args struct.
 * 
 * @return void
 */
{
    // Parse args
    algo_args* a_args = (algo_args*)args;

    #ifdef ERROR_CHECK
        if(a_args->src == NULL)
            error("algo_decode_lossless: src is NULL");
    #endif

    //Decode using specified encoding format
    a_args->dec_fun(a_args->z, *a_args->src, a_args->src_len, a_args->dest, a_args->dest_len, a_args->tmp);

    /* Lossless, don't touch anything */

    return;
}

void
algo_decode_cast32_64d (void* args)
{
    // Parse args
    algo_args* a_args = (algo_args*)args;

    char* decoded = NULL;
    size_t decoded_len = 0;

    // Decode using specified encoding format
    a_args->dec_fun(a_args->z, *a_args->src, a_args->src_len, &decoded, &decoded_len, a_args->tmp);

    // Deternmine length of data based on data format
    uint16_t len;
    float* res;
    
    #ifdef ERROR_CHECK
        if(a_args->src_format != _64d_) // non-essential check, but useful for debugging
            error("algo_decode_cast32_64d: Unknown data format");
    #endif
    len = decoded_len / sizeof(double);
    res = malloc((len + 1) * sizeof(float)); // Allocate space for result and leave room for header
    
    #ifdef ERROR_CHECK
        if(res == NULL)
            error("algo_decode_cast32: malloc failed");
    #endif

    double* f = (double*)(decoded + ZLIB_SIZE_OFFSET); // Ignore header in first 4 bytes
    for(int i = 1; i < len + 1; i++)
    {
        res[i] = (float)f[i-1];
    }

    // Store length of array in first 4 bytes
    res[0] = (float)len;

    // Return result
    *a_args->dest = res;
    *a_args->dest_len = (len +1 ) * sizeof(float);

    return;
}

void
algo_decode_log_2_transform_32f (void* args)
/**
 * @brief Log2 transform decoding function.
 * 
 * @param args Pointer to algo_args struct.
 * 
 * @return void
 */
{
    // Parse args
    algo_args* a_args = (algo_args*)args;

    char* decoded = NULL;
    size_t decoded_len = 0;

    //Decode using specified encoding format
    a_args->dec_fun(a_args->z, *a_args->src, a_args->src_len, &decoded, &decoded_len, a_args->tmp);

    // Deternmine length of data based on data format
    uint16_t len;
    uint16_t* res;

    #ifdef ERROR_CHECK
        if (a_args->src_format != _32f_) // non-essential check, but useful for debugging
            error("algo_decode_log_2_transform_32f: Unknown data format");
    #endif

    len = decoded_len / sizeof(float);

    // Perform log2 transform
    res = malloc((len + 1) * sizeof(uint16_t)); // Allocate space for result and leave room for header

    #ifdef ERROR_CHECK
        if(res == NULL)
            error("algo_decode_log_2_transform: malloc failed");
    #endif
    double ltran;

    float* f = (float*)(decoded + ZLIB_SIZE_OFFSET); // Ignore header in first 4 bytes
    uint16_t* tmp = (uint16_t*)(res) + 1; // Ignore header in first 4 bytes
    
    for(int i = 0; i < len; i++)
    {
        ltran = log2(f[i]);
        tmp[i] = floor(ltran * 100);
    }    

    // Free decoded buffer
    free(decoded);

    // Store length of array in first 4 bytes
    res[0] = len;

    // Return result
    *a_args->dest = res;
    *a_args->dest_len = (len + 1) * sizeof(uint16_t);

    return;
}

void
algo_decode_log_2_transform_64d (void* args)
{
       // Parse args
    algo_args* a_args = (algo_args*)args;

    char* decoded = NULL;
    size_t decoded_len = 0;

    //Decode using specified encoding format
    a_args->dec_fun(a_args->z, *a_args->src, a_args->src_len, &decoded, &decoded_len, a_args->tmp);

    // Deternmine length of data based on data format
    uint16_t len;
    uint16_t* res;

    #ifdef ERROR_CHECK
        if(a_args->src_format != _64d_) // non-essential check, but useful for debugging
            error("algo_decode_log_2_transform_64d: Unknown data format");
    #endif

    len = decoded_len / sizeof(double);

    // Perform log2 transform
    res = malloc((len + 1) * sizeof(uint16_t)); // Allocate space for result and leave room for header

    #ifdef ERROR_CHECK
        if(res == NULL)
            error("algo_decode_log_2_transform: malloc failed");
    #endif

    double ltran;

    double* f = (double*)(decoded + ZLIB_SIZE_OFFSET); // Ignore header in first 4 bytes
    uint16_t* tmp = (uint16_t*)(res) + 1; // Ignore header in first 4 bytes
    
    for(int i = 0; i < len; i++)
    {
        ltran = log2(f[i]);
        tmp[i] = floor(ltran * 100);
    }

    // Free decoded buffer
    free(decoded);

    // Store length of array in first 4 bytes
    res[0] = len;

    // Return result
    *a_args->dest = res;
    *a_args->dest_len = (len + 1) * sizeof(uint16_t);

    return;
}

void
algo_decode_delta16_transform_32f (void* args)
{
    // Parse args
    algo_args* a_args = (algo_args*)args;

    char* decoded = NULL;
    size_t decoded_len = 0;

    //Decode using specified encoding format
    a_args->dec_fun(a_args->z, *a_args->src, a_args->src_len, &decoded, &decoded_len, a_args->tmp);

    // Deternmine length of data based on data format
    uint16_t len;
    uint16_t* res;

    #ifdef ERROR_CHECK
        if(a_args->src_format != _32f_) // non-essential check, but useful for debugging
            error("algo_decode_delta_transform_32f: Unknown data format");
    #endif

    len = decoded_len / sizeof(float);

    size_t res_len = (len * sizeof(uint16_t)) + ZLIB_SIZE_OFFSET + sizeof(float);

    // Perform delta transform
    res = malloc(res_len); // Allocate space for result and leave room for header and first value

    #ifdef ERROR_CHECK
        if(res == NULL)
            error("algo_decode_delta_transform: malloc failed");
    #endif

    float* f = (float*)(decoded + ZLIB_SIZE_OFFSET); // Ignore header in first 4 bytes
    uint16_t* tmp = (uint16_t*)(res) + 1; // Ignore header in first 4 bytes

    //Store first value with full 32-bit precision
    // *(float*)&res[0] = f[0];
    memcpy(tmp, f, sizeof(float));

    // Perform delta transform
    for(int i = 1; i < len; i++)
    {
        float diff = f[i] - f[i-1];
        // uint16_t uint_diff = (diff > 0) ? (uint16_t)floor(diff) : 0; // clamp to 0 if diff is negative
        uint16_t uint_diff = (uint16_t)floor(diff * DELTA_SCALE_FACTOR); // scale by 2^16 / 10
        tmp[i+1] = uint_diff;
    }

    // Free decoded buffer
    free(decoded);

    // Store length of array in first 4 bytes
    res[0] = len;

    // Return result
    *a_args->dest = res;
    *a_args->dest_len = res_len;

    return;
}

void
algo_decode_delta16_transform_64d (void* args)
{
    // Parse args
    algo_args* a_args = (algo_args*)args;

    char* decoded = NULL;
    size_t decoded_len = 0;

    //Decode using specified encoding format
    a_args->dec_fun(a_args->z, *a_args->src, a_args->src_len, &decoded, &decoded_len, a_args->tmp);

    // Deternmine length of data based on data format
    uint16_t len;
    uint16_t* res;

    #ifdef ERROR_CHECK
        if(a_args->src_format != _64d_) // non-essential check, but useful for debugging
            error("algo_decode_delta_transform_64d: Unknown data format");
    #endif

    len = decoded_len / sizeof(double);

    size_t res_len = (len * sizeof(uint16_t)) + ZLIB_SIZE_OFFSET + sizeof(double);

    // Perform delta transform
    res = malloc(res_len); // Allocate space for result and leave room for header and first value

    #ifdef ERROR_CHECK
        if(res == NULL)
            error("algo_decode_delta_transform: malloc failed");
    #endif

    double* f = (double*)(decoded + ZLIB_SIZE_OFFSET); // Ignore header in first 4 bytes
    uint16_t* tmp = (uint16_t*)(res + 1); // Ignore header in first 4 bytes

    //Store first value with full 32-bit precision
    // *(float*)&res[0] = f[0];
    memcpy(tmp, f, sizeof(double));

    tmp += 4; // Move pointer to next value

    // Perform delta transform
    float diff;
    uint16_t uint_diff;
    for(int i = 1; i < len; i++)
    {
        diff = f[i] - f[i-1];
        uint_diff = (uint16_t)floor(diff * DELTA_SCALE_FACTOR); // scale by 2^16 / 10
        tmp[i-1] = uint_diff;
    }

    // Free decoded buffer
    free(decoded);

    // Store length of array in first 4 bytes
    res[0] = len;

    // Return result
    *a_args->dest = res;
    *a_args->dest_len = res_len;

    return;
}

void
algo_decode_delta32_transform_32f (void* args)
{
    // Parse args
    algo_args* a_args = (algo_args*)args;

    char* decoded = NULL;
    size_t decoded_len = 0;

    //Decode using specified encoding format
    a_args->dec_fun(a_args->z, *a_args->src, a_args->src_len, &decoded, &decoded_len, a_args->tmp);

    // Deternmine length of data based on data format
    uint32_t len;
    uint32_t* res;

    #ifdef ERROR_CHECK
        if(a_args->src_format != _32f_) // non-essential check, but useful for debugging
            error("algo_decode_delta_transform_32f: Unknown data format");
    #endif

    len = decoded_len / sizeof(float);

    size_t res_len = (len * sizeof(uint32_t)) + ZLIB_SIZE_OFFSET + sizeof(float);

    // Perform delta transform
    res = malloc(res_len); // Allocate space for result and leave room for header and first value

    #ifdef ERROR_CHECK
        if(res == NULL)
            error("algo_decode_delta_transform: malloc failed");
    #endif

    float* f = (float*)(decoded + ZLIB_SIZE_OFFSET); // Ignore header in first 4 bytes
    uint32_t* tmp = (uint32_t*)(res) + 1; // Ignore header in first 4 bytes

    //Store first value with full 32-bit precision
    // *(float*)&res[0] = f[0];
    memcpy(tmp, f, sizeof(float));

    // Perform delta transform
    for(int i = 1; i < len; i++)
    {
        float diff = f[i] - f[i-1];
        // uint16_t uint_diff = (diff > 0) ? (uint16_t)floor(diff) : 0; // clamp to 0 if diff is negative
        uint32_t uint_diff = (uint32_t)floor(diff * DELTA_SCALE_FACTOR); // scale by 2^16 / 10
        tmp[i+1] = uint_diff;
    }

    // Free decoded buffer
    free(decoded);

    // Store length of array in first 4 bytes
    res[0] = len;

    // Return result
    *a_args->dest = res;
    *a_args->dest_len = res_len;

    return;
}
/*
    @section Encoding functions
*/

void
algo_encode_lossless (void* args)
/**
 * @brief Lossless encoding function.
 * 
 * @param args Pointer to algo_args struct.
 * 
 * @return void
 */
{
    // Parse args
    algo_args* a_args = (algo_args*)args;
    
    #ifdef ERROR_CHECK
        if(a_args == NULL)
            error("algo_encode_lossless: args is NULL");
    #endif

    /* Lossless, don't touch anything */

    // Encode using specified encoding format
    a_args->enc_fun(a_args->z, a_args->src, a_args->src_len, a_args->dest, a_args->dest_len);

    return;
}

void
algo_encode_cast32_64d (void* args)
/**
 * @brief Casts 32-bit float array to 64-bit double array.
 * 
 * @param args Pointer to algo_args struct.
 * 
 * @return void
 */
{
    // Parse args
    algo_args* a_args = (algo_args*)args;

    #ifdef ERROR_CHECK
        if(a_args == NULL)
            error("algo_encode_cast32: args is NULL");
    #endif

    // Cast 32-bit to 64-bit 
    
    // Get source array 
    float* arr = (float*)(*a_args->src);
    
    // Get array length
    u_int16_t len = (uint16_t)arr[0];


    #ifdef ERROR_CHECK
        if (len <= 0)
            error("algo_encode_cast32: len is <= 0");
    #endif

    #ifdef ERROR_CHECK
        if (a_args->src_format != _64d_) // non-essential check, but useful for debugging
            error("algo_encode_cast32_64d: Unknown data format");
    #endif

    // Allocate buffer
    void* res = malloc(sizeof(double) * len);

    #ifdef ERROR_CHECK
        if(res == NULL)
            error("algo_encode_cast32: malloc failed");
    #endif

    double* res_arr = (double*)res;

    // Cast all
    for (size_t i = 1; i < len + 1; i++)
        res_arr[i-1] = (double)arr[i];
    
    // Encode using specified encoding format
    a_args->enc_fun(a_args->z, &res, len*sizeof(double), a_args->dest, a_args->dest_len);

    // Move src pointer
    *a_args->src += (len+1)*sizeof(float);

    // // Free buffer
    // free(res);
    return;
}

void
algo_encode_log_2_transform_32f (void* args)
{
    // Parse args
    algo_args* a_args = (algo_args*)args;

    #ifdef ERROR_CHECK
        if(a_args == NULL)
            error("algo_encode_log_2_transform: args is NULL");
    #endif

    // Get array length
    u_int16_t len = *(uint16_t*)(*a_args->src);

    #ifdef ERROR_CHECK
        if (len <= 0)
            error("algo_encode_log_2_transform: len is <= 0");
    #endif

    // Get source array 
    uint16_t* arr = (uint16_t*)((void*)(*a_args->src) + sizeof(u_int16_t));

    #ifdef ERROR_CHECK
        if (a_args->src_format != _32f_) // non-essential check, but useful for debugging
            error("algo_encode_log_2_transform: Unknown data format");
    #endif

    // Allocate buffer
    size_t res_len = len * sizeof(float);
    float* res = malloc(res_len);
    
    #ifdef ERROR_CHECK
        if(res == NULL)
            error("algo_encode_log_2_transform: malloc failed");
    #endif

    // Perform log2 transform
    for (size_t i = 0; i < len; i++)
        res[i] = (float)exp2((double)arr[i] / 100);

    // Encode using specified encoding format
    a_args->enc_fun(a_args->z, (char**)(&res), res_len, a_args->dest, a_args->dest_len);

    // Move to next array
    *a_args->src += (len * sizeof(uint16_t)) + ZLIB_SIZE_OFFSET;    

    return;
}

void
algo_encode_log_2_transform_64d (void* args)
{
    // Parse args
    algo_args* a_args = (algo_args*)args;

    #ifdef ERROR_CHECK
        if(a_args == NULL)
            error("algo_encode_log_2_transform: args is NULL");
    #endif

    // Get array length
    u_int16_t len = *(uint16_t*)(*a_args->src);

    #ifdef ERROR_CHECK
        if (len <= 0)
            error("algo_encode_log_2_transform: len is <= 0");
    #endif

    // Get source array 
    uint16_t* arr = (uint16_t*)((void*)(*a_args->src) + sizeof(u_int16_t));

    #ifdef ERROR_CHECK
        if (a_args->src_format != _64d_) // non-essential check, but useful for debugging
            error("algo_encode_log_2_transform: Unknown data format");
    #endif

   // Allocate buffer
    size_t res_len = len * sizeof(double);
    double* res = malloc(res_len);
    if(res == NULL)
        error("algo_encode_log_2_transform: malloc failed");
    // Perform log2 transform
    for (size_t i = 0; i < len; i++)
        res[i] = (double)exp2((double)arr[i] / 100);
        
    // Encode using specified encoding format
    a_args->enc_fun(a_args->z, (char**)(&res), res_len, a_args->dest, a_args->dest_len);

    // Move to next array
    *a_args->src += (len*sizeof(uint16_t)) + ZLIB_SIZE_OFFSET;

    // Encode using specified encoding format
    a_args->enc_fun(a_args->z, (char**)(&res), res_len, a_args->dest, a_args->dest_len);

    // Move to next array
    *a_args->src += (len * sizeof(uint16_t)) + ZLIB_SIZE_OFFSET;

    return;
}

void
algo_encode_delta16_transform_32f (void* args)
{
    // Parse args
    algo_args* a_args = (algo_args*)args;

    #ifdef ERROR_CHECK
        if(a_args == NULL)
            error("algo_encode_delta_transform: args is NULL");
    #endif

    // Get array length
    u_int16_t len = *(uint16_t*)(*a_args->src);

    #ifdef ERROR_CHECK
        if (len <= 0)
            error("algo_encode_delta_transform: len is <= 0");
    #endif

    // Get starting value
    float start = *(float*)((void*)(*a_args->src) + ZLIB_SIZE_OFFSET);

    // Get source array
    uint16_t* arr = (uint16_t*)((void*)(*a_args->src) + ZLIB_SIZE_OFFSET + sizeof(float));

    #ifdef ERROR_CHECK
        if (a_args->src_format != _32f_) // non-essential check, but useful for debugging
            error("algo_encode_delta_transform: Unknown data format");
    #endif

    // Allocate buffer
    size_t res_len = len * sizeof(float);
    float* res = malloc(res_len);

    #ifdef ERROR_CHECK
        if(res == NULL)
            error("algo_encode_delta_transform: malloc failed");
    #endif

    // Perform delta transform
    res[0] = start;
    for(size_t i = 1; i < len; i++)
        res[i] = res[i-1] + ((float)arr[i] / DELTA_SCALE_FACTOR);
    
    // Encode using specified encoding format
    a_args->enc_fun(a_args->z, (char**)(&res), res_len, a_args->dest, a_args->dest_len);

    // Move to next array
    *a_args->src += (len * sizeof(uint16_t)) + ZLIB_SIZE_OFFSET + sizeof(float);

    return;
}

void
algo_encode_delta16_transform_64d (void* args)
{
    // Parse args
    algo_args* a_args = (algo_args*)args;

    #ifdef ERROR_CHECK
        if(a_args == NULL)
            error("algo_encode_delta_transform: args is NULL");
    #endif

    // Get array length
    u_int16_t len = *(uint16_t*)(*a_args->src);

    #ifdef ERROR_CHECK
        if (len <= 0)
            error("algo_encode_delta_transform: len is <= 0");
    #endif

    // Get starting value
    double start = *(double*)((void*)(*a_args->src) + ZLIB_SIZE_OFFSET);

    // Get source array
    uint16_t* arr = (uint16_t*)((void*)(*a_args->src) + ZLIB_SIZE_OFFSET + sizeof(double));

    #ifdef ERROR_CHECK
        if (a_args->src_format != _64d_) // non-essential check, but useful for debugging
            error("algo_encode_delta_transform: Unknown data format");
    #endif

    // Allocate buffer
    size_t res_len = len * sizeof(double);
    double* res = malloc(res_len);

    #ifdef ERROR_CHECK
        if(res == NULL)
            error("algo_encode_delta_transform: malloc failed");
    #endif

    // Perform delta transform
    res[0] = start;
    for(size_t i = 1; i < len; i++)
        res[i] = res[i-1] + ((double)arr[i] / DELTA_SCALE_FACTOR);
    
    // Encode using specified encoding format
    a_args->enc_fun(a_args->z, (char**)(&res), res_len, a_args->dest, a_args->dest_len);

    // Move to next array
    *a_args->src += (len * sizeof(uint16_t)) + ZLIB_SIZE_OFFSET + sizeof(double);

    return;
}

void
algo_encode_delta32_transform_32f (void* args)
{
    // Parse args
    algo_args* a_args = (algo_args*)args;

    #ifdef ERROR_CHECK
        if(a_args == NULL)
            error("algo_encode_delta_transform: args is NULL");
    #endif

    // Get array length
    u_int32_t len = *(uint32_t*)(*a_args->src);

    #ifdef ERROR_CHECK
        if (len <= 0)
            error("algo_encode_delta_transform: len is <= 0");
    #endif

    // Get starting value
    float start = *(float*)((void*)(*a_args->src) + ZLIB_SIZE_OFFSET);

    // Get source array
    uint32_t* arr = (uint32_t*)((void*)(*a_args->src) + ZLIB_SIZE_OFFSET + sizeof(float));

    #ifdef ERROR_CHECK
        if (a_args->src_format != _32f_) // non-essential check, but useful for debugging
            error("algo_encode_delta_transform: Unknown data format");
    #endif

    // Allocate buffer
    size_t res_len = len * sizeof(float);
    float* res = malloc(res_len);

    #ifdef ERROR_CHECK
        if(res == NULL)
            error("algo_encode_delta_transform: malloc failed");
    #endif

    // Perform delta transform
    res[0] = start;
    for(size_t i = 1; i < len; i++)
        res[i] = res[i-1] + ((float)arr[i] / DELTA_SCALE_FACTOR);
    
    // Encode using specified encoding format
    a_args->enc_fun(a_args->z, (char**)(&res), res_len, a_args->dest, a_args->dest_len);

    // Move to next array
    *a_args->src += (len * sizeof(uint32_t)) + ZLIB_SIZE_OFFSET + sizeof(float);

    return;
}
/*
    @section Algo switch
*/

Algo_ptr
set_compress_algo(int algo, int accession)
{
    switch(algo)
    {
        case _lossless_ :       return algo_decode_lossless;
        case _log2_transform_ :
        {
            switch(accession)
            {
                case _32f_ :    return algo_decode_log_2_transform_32f;
                case _64d_ :    return algo_decode_log_2_transform_64d;
            }
        } ;
        case _cast_64_to_32_:
        {   
            switch(accession)
            {
                case _64d_ :    return algo_decode_cast32_64d;
                case _32f_ :    return algo_decode_lossless; // casting 32 to 32 is just lossless
            }
        } ;
        case _delta16_transform_ :
        {
            switch(accession)
            {
                case _32f_ :    return algo_decode_delta16_transform_32f;
                case _64d_ :    return algo_decode_delta16_transform_64d;
            }
        } ;
        case _delta32_transform_ :
        {
            switch(accession)
            {
                case _32f_ :    return algo_decode_delta32_transform_32f;
                // case _64d_ :    return algo_decode_delta32_transform_64d;
                case _64d_ :    assert(0); // not implemented yet
            }
        } ;
        default:                error("set_compress_algo: Unknown compression algorithm");
    }
}

Algo_ptr
set_decompress_algo(int algo, int accession)
{   
    switch(algo)
    {
        case _lossless_ :       return algo_encode_lossless;
        case _log2_transform_ : 
        {
            switch(accession)
            {
                case _32f_ :    return algo_encode_log_2_transform_32f;
                case _64d_ :    return algo_encode_log_2_transform_64d;
            }
        } ;
        case _cast_64_to_32_:   
        {
            switch(accession)
            {
                case _64d_ :    return algo_encode_cast32_64d;
                case _32f_ :    return algo_encode_lossless; // casting 32 to 32 is just lossless
            }
        } ;
        case _delta16_transform_ :
        {
            switch(accession)
            {
                case _32f_ :    return algo_encode_delta16_transform_32f;
                case _64d_ :    return algo_encode_delta16_transform_64d;
            }
        } ;
        case _delta32_transform_ :
        {
            switch(accession)
            {
                case _32f_ :    return algo_encode_delta32_transform_32f;
                // case _64d_ :    return algo_encode_delta32_transform_64d;
                case _64d_ :    assert(0); // not implemented yet
            }
        } ;
        default:                error("set_decompress_algo: Unknown compression algorithm");
    }
}

int
get_algo_type(char* arg)
{
    if(arg == NULL)
        error("get_algo_type: arg is NULL");
    if(strcmp(arg, "lossless") == 0 || *arg == '\0' || *arg == "")
        return _lossless_;
    else if(strcmp(arg, "log") == 0)
        return _log2_transform_;
    else if(strcmp(arg, "cast") == 0)
        return _cast_64_to_32_;
    else if(strcmp(arg, "delta16") == 0)
        return _delta16_transform_;
    else if(strcmp(arg, "delta32") == 0)
        return _delta32_transform_;
    else
        error("get_algo_type: Unknown compression algorithm");
}