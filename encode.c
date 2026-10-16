
#include <stdio.h>
#include <string.h>
#include "encode.h"
#include "common.h"

// Encode one byte into the image LSBs
static Status encode_byte_to_lsb(unsigned char data,
                                 FILE *src, FILE *dest)
{
    unsigned char image_byte;
    int bit;

    for (bit = 7; bit >= 0; bit--)
    {
        if (fread(&image_byte, 1, 1, src) != 1)
            return e_failure;

        image_byte = (unsigned char)(
            (image_byte & 0xFE) | ((data >> bit) & 1));

        if (fwrite(&image_byte, 1, 1, dest) != 1)
            return e_failure;
    }

    return e_success;
}

// Encode an integer byte by byte
static Status encode_int(uint value, FILE *src, FILE *dest)
{
    int i;

    for (i = 3; i >= 0; i--)
    {
        unsigned char byte =
            (unsigned char)((value >> (i * 8)) & 0xFF);

        if (encode_byte_to_lsb(byte, src, dest) != e_success)
            return e_failure;
    }

    return e_success;
}

// Validate filenames and extract the secret file extension
Status read_and_validate_encode_args(char *argv[], EncodeInfo *encInfo)
{
    const char *dot;
    size_t ext_len;

    encInfo->src_image_fname = argv[2];
    encInfo->secret_fname = argv[3];

    if (argv[4] != NULL)
        encInfo->stego_image_fname = argv[4];

    dot = strrchr(encInfo->src_image_fname, '.');

    if (dot == NULL || strcmp(dot, ".bmp") != 0)
        return e_failure;

    if (strcmp(encInfo->src_image_fname, encInfo->stego_image_fname) == 0 ||
        strcmp(encInfo->secret_fname, encInfo->stego_image_fname) == 0)
        return e_failure;

    dot = strrchr(encInfo->secret_fname, '.');

    if (dot == NULL || dot[1] == '\0')
        return e_failure;

    ext_len = strlen(dot + 1);

    if (ext_len > 4)
        return e_failure;

    strcpy(encInfo->extn_secret_file, dot + 1);

    return e_success;
}

// Open source, secret and output files
Status open_files(EncodeInfo *encInfo)
{
    long file_size;

    encInfo->fptr_src_image =
        fopen(encInfo->src_image_fname, "rb");

    if (encInfo->fptr_src_image == NULL)
        goto error;

    encInfo->fptr_secret = fopen(encInfo->secret_fname, "rb");

    if (encInfo->fptr_secret == NULL)
        goto error;

    if (fseek(encInfo->fptr_secret, 0, SEEK_END) != 0)
        goto error;

    file_size = ftell(encInfo->fptr_secret);

    if (file_size < 0 || (unsigned long)file_size > 0xFFFFFFFFUL)
        goto error;

    encInfo->size_secret_file = (uint)file_size;
    rewind(encInfo->fptr_secret);

    encInfo->fptr_stego_image =
        fopen(encInfo->stego_image_fname, "wb");

    if (encInfo->fptr_stego_image == NULL)
        goto error;

    return e_success;

error:
    if (encInfo->fptr_src_image != NULL)
    {
        fclose(encInfo->fptr_src_image);
        encInfo->fptr_src_image = NULL;
    }

    if (encInfo->fptr_secret != NULL)
    {
        fclose(encInfo->fptr_secret);
        encInfo->fptr_secret = NULL;
    }

    if (encInfo->fptr_stego_image != NULL)
    {
        fclose(encInfo->fptr_stego_image);
        encInfo->fptr_stego_image = NULL;
    }

    return e_failure;
}

// Check whether the image has enough capacity
Status check_capacity(EncodeInfo *encInfo)
{
    long image_size;
    unsigned long long available_bytes;
    unsigned long long required_bytes;
    size_t ext_len = strlen(encInfo->extn_secret_file);

    if (fseek(encInfo->fptr_src_image, 0, SEEK_END) != 0)
        return e_failure;

    image_size = ftell(encInfo->fptr_src_image);

    if (image_size < BMP_HEADER_SIZE)
        return e_failure;

    available_bytes =
        (unsigned long long)(image_size - BMP_HEADER_SIZE);

    required_bytes =
        (unsigned long long)strlen(MAGIC_STRING) +
        4ULL + (unsigned long long)ext_len +
        4ULL + (unsigned long long)encInfo->size_secret_file;

    required_bytes *= 8ULL;

    encInfo->image_capacity =
        available_bytes > 0xFFFFFFFFULL
        ? 0xFFFFFFFFU : (uint)available_bytes;

    rewind(encInfo->fptr_src_image);

    if (required_bytes > available_bytes)
        return e_failure;

    return e_success;
}

// Copy the BMP header unchanged
Status copy_bmp_header(FILE *fptr_src, FILE *fptr_dest)
{
    unsigned char header[BMP_HEADER_SIZE];

    if (fread(header, 1, BMP_HEADER_SIZE, fptr_src) != BMP_HEADER_SIZE)
        return e_failure;

    if (fwrite(header, 1, BMP_HEADER_SIZE, fptr_dest) != BMP_HEADER_SIZE)
        return e_failure;

    return e_success;
}

// Encode the magic string
Status encode_magic_string(const char *magic_string, EncodeInfo *encInfo)
{
    size_t i;

    for (i = 0; i < strlen(magic_string); i++)
    {
        if (encode_byte_to_lsb((unsigned char)magic_string[i],
                encInfo->fptr_src_image,
                encInfo->fptr_stego_image) != e_success)
            return e_failure;
    }

    return e_success;
}

// Encode extension size first, then the extension
Status encode_secret_file_extn(EncodeInfo *encInfo)
{
    size_t i;
    uint ext_len = (uint)strlen(encInfo->extn_secret_file);

    // Encode extension size
    if (encode_int(ext_len, encInfo->fptr_src_image,
                   encInfo->fptr_stego_image) != e_success)
        return e_failure;

    printf("Encoded secret file extension size successfully. ✓\n");

    // Encode extension characters
    for (i = 0; i < ext_len; i++)
    {
        if (encode_byte_to_lsb(
                (unsigned char)encInfo->extn_secret_file[i],
                encInfo->fptr_src_image,
                encInfo->fptr_stego_image) != e_success)
            return e_failure;
    }

    printf("Encoded secret file extension successfully. ✓\n");

    return e_success;
}

// Encode the secret file size
Status encode_secret_file_size(uint size, EncodeInfo *encInfo)
{
    return encode_int(size, encInfo->fptr_src_image,
                      encInfo->fptr_stego_image);
}

// Encode the secret file contents
Status encode_secret_file_data(EncodeInfo *encInfo)
{
    unsigned char ch;

    rewind(encInfo->fptr_secret);

    while (fread(&ch, 1, 1, encInfo->fptr_secret) == 1)
    {
        if (encode_byte_to_lsb(ch, encInfo->fptr_src_image,
                               encInfo->fptr_stego_image) != e_success)
            return e_failure;
    }

    return ferror(encInfo->fptr_secret) ? e_failure : e_success;
}

// Copy the remaining image data
Status copy_remaining_img_data(FILE *fptr_src, FILE *fptr_dest)
{
    unsigned char buffer[4096];
    size_t count;

    while ((count = fread(buffer, 1, sizeof(buffer), fptr_src)) > 0)
    {
        if (fwrite(buffer, 1, count, fptr_dest) != count)
            return e_failure;
    }

    return ferror(fptr_src) ? e_failure : e_success;
}

// Perform the encoding process
Status do_encoding(EncodeInfo *encInfo)
{
    Status status = e_failure;

    if (open_files(encInfo) != e_success)
        goto cleanup;

    printf("Opened files successfully. ✓\n");

    if (check_capacity(encInfo) != e_success)
    {
        printf("ERROR: Image does not have enough capacity.\n");
        goto cleanup;
    }

    printf("Checked capacity successfully. ✓\n");

    rewind(encInfo->fptr_src_image);

    if (copy_bmp_header(encInfo->fptr_src_image,
                        encInfo->fptr_stego_image) != e_success)
        goto cleanup;

    printf("Copied image header successfully. ✓\n");

    if (encode_magic_string(MAGIC_STRING, encInfo) != e_success)
        goto cleanup;

    printf("Encoded magic string successfully. ✓\n");

    // Encode extension size and extension
    if (encode_secret_file_extn(encInfo) != e_success)
        goto cleanup;

    // Encode secret file data size
    if (encode_secret_file_size(encInfo->size_secret_file,
                                encInfo) != e_success)
        goto cleanup;

    printf("Encoded secret file data size successfully. ✓\n");

    // Encode secret file contents
    if (encode_secret_file_data(encInfo) != e_success)
        goto cleanup;

    printf("Encoded secret file data successfully. ✓\n");

    // Copy remaining image data
    if (copy_remaining_img_data(encInfo->fptr_src_image,
                                encInfo->fptr_stego_image) != e_success)
        goto cleanup;

    printf("Copied remaining image data successfully. ✓\n");

    status = e_success;

    printf("\nEncoding completed successfully. ✓\n");
    printf("Output image: %s\n", encInfo->stego_image_fname);

cleanup:
    if (encInfo->fptr_src_image != NULL)
    {
        fclose(encInfo->fptr_src_image);
        encInfo->fptr_src_image = NULL;
    }

    if (encInfo->fptr_secret != NULL)
    {
        fclose(encInfo->fptr_secret);
        encInfo->fptr_secret = NULL;
    }

    if (encInfo->fptr_stego_image != NULL)
    {
        fclose(encInfo->fptr_stego_image);
        encInfo->fptr_stego_image = NULL;
    }

    return status;
}
