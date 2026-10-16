
#include <stdio.h>
#include <string.h>
#include "decode.h"
#include "common.h"

// Decode one byte from image LSBs
static Status decode_byte_from_lsb(unsigned char *data, FILE *image)
{
    unsigned char image_byte;
    int bit;

    *data = 0;

    for (bit = 7; bit >= 0; bit--)
    {
        if (fread(&image_byte, 1, 1, image) != 1)
            return e_failure;

        *data = (unsigned char)((*data << 1) | (image_byte & 1));
    }

    return e_success;
}

// Decode a four-byte integer
static Status decode_int(uint *value, FILE *image)
{
    unsigned char byte;
    int i;

    *value = 0;

    for (i = 0; i < 4; i++)
    {
        if (decode_byte_from_lsb(&byte, image) != e_success)
            return e_failure;

        *value = (*value << 8) | byte;
    }

    return e_success;
}

// Open the encoded image
Status open_decode_files(DecodeInfo *decInfo)
{
    decInfo->fptr_stego_image =
        fopen(decInfo->stego_image_fname, "rb");

    if (decInfo->fptr_stego_image == NULL)
    {
        perror("Encoded image");
        return e_failure;
    }

    return e_success;
}

// Decode and verify the magic string
Status decode_magic_string(DecodeInfo *decInfo)
{
    char magic[sizeof(MAGIC_STRING)];
    size_t i;

    for (i = 0; i < strlen(MAGIC_STRING); i++)
    {
        unsigned char ch;

        if (decode_byte_from_lsb(&ch,
                decInfo->fptr_stego_image) != e_success)
            return e_failure;

        magic[i] = (char)ch;
    }

    magic[i] = '\0';

    return strcmp(magic, MAGIC_STRING) == 0
           ? e_success : e_failure;
}

// Decode extension size first, then the extension
Status decode_secret_file_extn(DecodeInfo *decInfo)
{
    uint ext_len;
    uint i;
    unsigned char ch;

    // Decode extension size
    if (decode_int(&ext_len, decInfo->fptr_stego_image) != e_success)
        return e_failure;

    printf("Decoded secret file extension size successfully. ✓\n");

    if (ext_len == 0 || ext_len > 4)
        return e_failure;

    // Decode extension characters
    for (i = 0; i < ext_len; i++)
    {
        if (decode_byte_from_lsb(&ch,
                decInfo->fptr_stego_image) != e_success)
            return e_failure;

        decInfo->extn_secret_file[i] = (char)ch;
    }

    decInfo->extn_secret_file[ext_len] = '\0';

    if (strchr(decInfo->extn_secret_file, '/') != NULL ||
        strchr(decInfo->extn_secret_file, '\\') != NULL ||
        strchr(decInfo->extn_secret_file, '.') != NULL)
        return e_failure;

    printf("Decoded secret file extension successfully. ✓\n");

    return e_success;
}

// Decode the secret file data size
Status decode_secret_file_size(DecodeInfo *decInfo)
{
    return decode_int(&decInfo->size_secret_file,
                      decInfo->fptr_stego_image);
}

// Create output filename using the supplied name or default name
Status create_output_filename(DecodeInfo *decInfo)
{
    char filename[256];
    char *base;
    char *slash1;
    char *slash2;
    char *token;
    int length;

    // Use "output" if no filename is supplied
    if (decInfo->requested_output_fname == NULL)
    {
        strcpy(filename, "output");
    }
    else
    {
        length = snprintf(filename, sizeof(filename), "%s",
                          decInfo->requested_output_fname);

        if (length < 0 || (size_t)length >= sizeof(filename))
            return e_failure;

        // Find the filename after the directory path
        slash1 = strrchr(filename, '/');
        slash2 = strrchr(filename, '\\');

        if (slash1 == NULL)
            base = slash2;
        else if (slash2 == NULL)
            base = slash1;
        else
            base = slash1 > slash2 ? slash1 : slash2;

        if (base != NULL)
            base++;
        else
            base = filename;

        // Remove the supplied extension using strtok()
        token = strtok(base, ".");

        if (token == NULL || token[0] == '\0')
            return e_failure;

        memmove(filename, token, strlen(token) + 1);
    }

    // Append the extension decoded from the image
    length = snprintf(decInfo->output_fname,
                      sizeof(decInfo->output_fname),
                      "%s.%s", filename,
                      decInfo->extn_secret_file);

    if (length < 0 ||
        (size_t)length >= sizeof(decInfo->output_fname))
        return e_failure;

    // Avoid overwriting the encoded image
    if (strcmp(decInfo->output_fname,
               decInfo->stego_image_fname) == 0)
        return e_failure;

    return e_success;
}

// Decode the actual secret file contents
Status decode_secret_file_data(DecodeInfo *decInfo)
{
    uint i;
    unsigned char ch;

    // Extract exactly the stored number of bytes
    for (i = 0; i < decInfo->size_secret_file; i++)
    {
        if (decode_byte_from_lsb(&ch,
                decInfo->fptr_stego_image) != e_success)
            return e_failure;

        if (fwrite(&ch, 1, 1, decInfo->fptr_output) != 1)
            return e_failure;
    }

    return e_success;
}

// Perform the decoding process
Status do_decoding(DecodeInfo *decInfo)
{
    Status status = e_failure;
    unsigned char header[BMP_HEADER_SIZE];

    if (open_decode_files(decInfo) != e_success)
        goto cleanup;

    printf("Opened stego image successfully. ✓\n");

    // Skip the BMP header
    if (fread(header, 1, BMP_HEADER_SIZE,
              decInfo->fptr_stego_image) != BMP_HEADER_SIZE)
        goto cleanup;

    printf("Skipped image header successfully. ✓\n");

    // Verify the magic string
    if (decode_magic_string(decInfo) != e_success)
    {
        printf("ERROR: Invalid magic string.\n");
        goto cleanup;
    }

    printf("Decoded magic string successfully. ✓\n");

    // Decode extension size and extension
    if (decode_secret_file_extn(decInfo) != e_success)
    {
        printf("ERROR: Unable to decode extension.\n");
        goto cleanup;
    }

    // Decode the secret data size
    if (decode_secret_file_size(decInfo) != e_success)
        goto cleanup;

    printf("Decoded secret file data size successfully. ✓\n");

    // Create the output filename
    if (create_output_filename(decInfo) != e_success)
    {
        printf("ERROR: Invalid output filename.\n");
        goto cleanup;
    }

    printf("Created output filename successfully. ✓\n");

    decInfo->fptr_output = fopen(decInfo->output_fname, "wb");

    if (decInfo->fptr_output == NULL)
    {
        perror("Output file");
        goto cleanup;
    }

    // Decode and save the actual secret data
    if (decode_secret_file_data(decInfo) != e_success)
        goto cleanup;

    printf("Decoded secret file data successfully. ✓\n");

    status = e_success;

    printf("\nDecoding completed successfully. ✓\n");
    printf("Output file: %s\n", decInfo->output_fname);

cleanup:
    if (decInfo->fptr_output != NULL)
    {
        fclose(decInfo->fptr_output);
        decInfo->fptr_output = NULL;
    }

    if (decInfo->fptr_stego_image != NULL)
    {
        fclose(decInfo->fptr_stego_image);
        decInfo->fptr_stego_image = NULL;
    }

    return status;
}
