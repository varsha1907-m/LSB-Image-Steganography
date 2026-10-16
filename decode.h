
#ifndef DECODE_H
#define DECODE_H

#include <stdio.h>
#include "types.h"

// Stores decoding information
typedef struct
{
    char *stego_image_fname;
    FILE *fptr_stego_image;

    char *requested_output_fname;
    char output_fname[256];
    FILE *fptr_output;

    char extn_secret_file[5];
    uint size_secret_file;

} DecodeInfo;

Status open_decode_files(DecodeInfo *decInfo);
Status decode_magic_string(DecodeInfo *decInfo);
Status decode_secret_file_extn(DecodeInfo *decInfo);
Status decode_secret_file_size(DecodeInfo *decInfo);
Status create_output_filename(DecodeInfo *decInfo);
Status decode_secret_file_data(DecodeInfo *decInfo);
Status do_decoding(DecodeInfo *decInfo);

#endif
