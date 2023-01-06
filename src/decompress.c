#include "mscompress.h"
#include <zstd.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <pthread.h>
#include <assert.h>

ZSTD_DCtx *
alloc_dctx()
/**
 * @brief Creates a ZSTD decompression context and handles errors.
 * 
 * @return A ZSTD decompression context on success. Exits on error.
 * 
 */
{
    ZSTD_DCtx* dctx = ZSTD_createDCtx();
    if(dctx == NULL) 
        error("alloc_dctx: ZSTD Context failed.\n");
    return dctx;
}

void *
alloc_ztsd_dbuff(size_t buff_len)
{
    void* r = malloc(buff_len);
    if(r == NULL)
        error("alloc_ztsd_dbuff: malloc() error.\n");
    return r;
}

void *
zstd_decompress(ZSTD_DCtx* dctx, void* src_buff, size_t src_len, size_t org_len)
{
    void* out_buff;
    size_t decmp_len = 0;

    out_buff = alloc_ztsd_dbuff(org_len); // will return buff, exit on error

    decmp_len = ZSTD_decompressDCtx(dctx, out_buff, org_len, src_buff, src_len);

    if (decmp_len != org_len)
        error("zstd_decompress: ZSTD_decompressDCtx() error: %s\n", ZSTD_getErrorName(decmp_len));

    return out_buff;

}

void *
decmp_block(ZSTD_DCtx* dctx, void* input_map, long offset, block_len_t* blk)
{
    return zstd_decompress(dctx, input_map+offset, blk->compressed_size, blk->original_size);
}

decompress_args_t*
alloc_decompress_args(char* input_map,
                      data_format_t* df,
                      block_len_t* xml_blk,
                      block_len_t* mz_binary_blk,
                      block_len_t* inten_binary_blk,
                      division_t* division,
                      off_t footer_xml_off,
                      off_t footer_mz_bin_off,
                      off_t footer_inten_bin_off)
{
    decompress_args_t* r;
    
    r = malloc(sizeof(decompress_args_t));
    if(r == NULL)
        error("alloc_decompress_args: malloc() error.\n");

    r->input_map = input_map;
    r->df = df;
    r->xml_blk = xml_blk;
    r->mz_binary_blk = mz_binary_blk;
    r->inten_binary_blk = inten_binary_blk;
    r->division = division;
    r->footer_xml_off = footer_xml_off;
    r->footer_mz_bin_off = footer_mz_bin_off;
    r->footer_inten_bin_off = footer_inten_bin_off;

    r->ret = NULL;
    r->ret_len = 0;

    return r;
}

void
dealloc_decompress_args(decompress_args_t* args)
{
    if(args)
    {
        if(args->ret) free(args->ret);
        free(args);
    }
}

int
get_lowest(int i_0, int i_1, int i_2)
{
    int ret = -1;

    if(i_0 < i_1 && i_0 < i_2) 
        ret = 0;
    else if (i_1 < i_0 && i_1 < i_2)
        ret = 1;
    else if (i_2 < i_0 && i_2 < i_1)
        ret = 2;    

    return ret;
}


void
decompress_routine(void* args)
{
    // Get thread ID
    int tid = get_thread_id();

    // Allocate a decompression context
    ZSTD_DCtx* dctx = alloc_dctx();

    if(dctx == NULL)
        error("decompress_routine: ZSTD Context failed.\n");

    decompress_args_t* db_args = (decompress_args_t*)args;
    division_t* division = db_args->division;

    if(db_args == NULL)
        error("decompress_routine: Decompression arguments are null.\n");

    // Decompress each block of data
    void
        *decmp_xml = decmp_block(dctx, db_args->input_map, db_args->footer_xml_off, db_args->xml_blk),
        *decmp_mz_binary = decmp_block(dctx, db_args->input_map, db_args->footer_mz_bin_off, db_args->mz_binary_blk),
        *decmp_inten_binary = decmp_block(dctx, db_args->input_map, db_args->footer_inten_bin_off, db_args->inten_binary_blk);

    size_t binary_len = 0;

    int64_t buff_off = 0,
            xml_off = 0, mz_off = 0, inten_off = 0;
    int64_t xml_i = 0,   mz_i = 0,   inten_i = 0;        

    int block = 0;

    long len = division->size;

    if(len <= 0)
        error("decompress_routine: Error determining decompression buffer size.\n");

    char* buff = malloc(len*2); // *2 to be safe TODO: fix this
    if(buff == NULL)
        error("decompress_routine: Failed to allocate buffer for decompression.\n");

    db_args->ret = buff;

    int64_t curr_len = 0;

    algo_args* a_args = malloc(sizeof(algo_args));
    if(a_args == NULL)
        error("decompress_routine: Failed to allocate algo_args.\n");

    size_t algo_output_len = 0;
    a_args->dest_len = &algo_output_len;
    a_args->enc_fun = db_args->df->encode_source_compression_fun;

    data_positions_t* curr_dp;

    while(block != -1)
    {
        switch (block)
        {
        case 0: // xml
            curr_dp = division->xml;
            if(xml_i == curr_dp->total_spec) {
                block = -1; break;}
            curr_len = curr_dp->end_positions[xml_i] - curr_dp->start_positions[xml_i];
            assert(curr_len > 0 && curr_len < len);
            memcpy(buff + buff_off, decmp_xml + xml_off, curr_len);
            xml_off += curr_len;
            buff_off += curr_len;
            xml_i++;
            block++;
            break;
        case 1: // mz
            curr_dp = division->mz;
            if(mz_i == curr_dp->total_spec) {
                block = 0; break;}
            curr_len = curr_dp->end_positions[mz_i] - curr_dp->start_positions[mz_i];
            assert(curr_len > 0 && curr_len < len);
            a_args->src = (char**)&decmp_mz_binary;
            a_args->src_len = curr_len;
            a_args->dest = buff+buff_off;
            a_args->src_format = db_args->df->source_mz_fmt;
            db_args->df->target_mz_fun((void*)a_args);
            buff_off += *a_args->dest_len;
            mz_i++;
            block++;
            break;
        case 2: // xml
            curr_dp = division->xml;
            if(xml_i == curr_dp->total_spec) {
                block = -1; break;}
            curr_len = curr_dp->end_positions[xml_i] - curr_dp->start_positions[xml_i];
            assert(curr_len > 0 && curr_len < len);
            memcpy(buff + buff_off, decmp_xml + xml_off, curr_len);
            xml_off += curr_len;
            buff_off += curr_len;
            xml_i++;
            block++;
            break;
        case 3: // int
            curr_dp = division->inten;
            if(inten_i == curr_dp->total_spec) {
                block = 0; break;}
            curr_len = curr_dp->end_positions[inten_i] - curr_dp->start_positions[inten_i];
            assert(curr_len > 0 && curr_len < len);
            a_args->src = (char**)&decmp_inten_binary;
            a_args->src_len = curr_len;
            a_args->dest = buff+buff_off;
            a_args->src_format = db_args->df->source_inten_fmt;
            db_args->df->target_inten_fun((void*)a_args);
            buff_off += *a_args->dest_len;
            inten_i++;
            block = 0;
            break;
        case -1:
            break;
        }
    }

    db_args->ret_len = buff_off;

    return;
}

void
decompress_parallel(char* input_map,
                    block_len_queue_t* xml_block_lens,
                    block_len_queue_t* mz_binary_block_lens,
                    block_len_queue_t* inten_binary_block_lens,
                    divisions_t* divisions,
                    data_format_t* df,
                    footer_t* msz_footer,
                    int threads, int fd)
{
    decompress_args_t* args[divisions->n_divisions];
    pthread_t ptid[divisions->n_divisions];

    block_len_t *xml_blk, *mz_binary_blk, *inten_binary_blk;

    off_t footer_xml_off = 0, footer_mz_bin_off = 0, footer_inten_bin_off = 0; // offset within corresponding data_block.

    int i;

    int divisions_used = 0;

    clock_t start, stop;
    
    while(divisions_used < divisions->n_divisions)
    {
        for(i = divisions_used; i < divisions_used + threads; i++)
        {
            xml_blk = pop_block_len(xml_block_lens);
            mz_binary_blk = pop_block_len(mz_binary_block_lens);
            inten_binary_blk = pop_block_len(inten_binary_block_lens);

            args[i] = alloc_decompress_args(input_map,
                                            df,
                                            xml_blk,
                                            mz_binary_blk,
                                            inten_binary_blk,
                                            divisions->divisions[i],
                                            footer_xml_off + msz_footer->xml_pos,
                                            footer_mz_bin_off + msz_footer->mz_binary_pos,
                                            footer_inten_bin_off + msz_footer->inten_binary_pos);

            footer_xml_off += xml_blk->compressed_size;
            footer_mz_bin_off += mz_binary_blk->compressed_size;
            footer_inten_bin_off += inten_binary_blk->compressed_size;
        }

        for(i = divisions_used; i < divisions_used + threads; i++)
        {
            int ret = pthread_create(&ptid[i], NULL, &decompress_routine, (void*)args[i]);
            if(ret != 0)
            {
                perror("pthread_create");
                exit(-1);
            }
        }

        for(i = divisions_used; i < divisions_used + threads; i++)
        {
            int ret = pthread_join(ptid[i], NULL);
            if(ret != 0)
            {
                perror("pthread_join");
                exit(-1);
            }

            start = clock();
            write_to_file(fd, args[i]->ret, args[i]->ret_len);
            stop = clock();

            print("\tWrote %ld bytes to disk (%1.2fmb/s)\n", args[i]->ret_len, ((double)args[i]->ret_len/1000000)/((double)(stop-start)/CLOCKS_PER_SEC));

            dealloc_decompress_args(args[i]);
        }
        divisions_used += threads;
    }
}