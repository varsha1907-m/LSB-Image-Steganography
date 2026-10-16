
#ifndef ENCODE_H
#define ENCODE_H

#include <stdio.h>
#include "types.h"

// Stores encoding information
typedef struct
{
    char *src_image_fname;
    FILE *fptr_src_image;

    char *secret_fname;
    FILE *fptr_secret;

    char *stego_image_fname;
    FILE *fptr_stego_image;

    uint image_capacity;
    uint size_secret_file;

    char extn_secret_file[5];

} EncodeInfo;

Status read_and_validate_encode_args(char *argv[], EncodeInfo *encInfo);
Status open_files(EncodeInfo *encInfo);
Status check_capacity(EncodeInfo *encInfo);
Status copy_bmp_header(FILE *fptr_src, FILE *fptr_dest);
Status encode_magic_string(const char *magic_string, EncodeInfo *encInfo);
Status encode_secret_file_extn(EncodeInfo *encInfo);
Status encode_secret_file_size(uint size, EncodeInfo *encInfo);
Status encode_secret_file_data(EncodeInfo *encInfo);
Status copy_remaining_img_data(FILE *fptr_src, FILE *fptr_dest);
Status do_encoding(EncodeInfo *encInfo);

#endif
