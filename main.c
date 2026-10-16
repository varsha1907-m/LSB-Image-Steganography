#include <stdio.h>
#include <string.h>
#include "types.h"
#include "encode.h"
#include "decode.h"

// Identify the requested operation
OperationType check_operation_type(char *argv[])
{
    if (strcmp(argv[1], "-e") == 0)
        return e_encode;

    if (strcmp(argv[1], "-d") == 0)
        return e_decode;

    return e_unsupported;
}

int main(int argc, char *argv[])
{
    OperationType operation;

    if (argc < 3)
    {
        printf("Usage:\n");
        printf("./a.out -e source.bmp secret.txt [output.bmp]\n");
        printf("./a.out -d encoded.bmp [output_filename]\n");
        return 1;
    }

    operation = check_operation_type(argv);

    if (operation == e_encode)
    {
        EncodeInfo encInfo = {0};

        if (argc != 4 && argc != 5)
            return 1;

        printf("\n========== LSB IMAGE STEGNOGRAPHY ==========\n");
        printf("\n--------------- ENCODING COMPLETE -------------------\n\n");

        encInfo.src_image_fname = argv[2];
        encInfo.secret_fname = argv[3];
        encInfo.stego_image_fname =
            (argc == 5) ? argv[4] : "stego.bmp";

        if (read_and_validate_encode_args(argv, &encInfo) != e_success)
            return 1;

        if (do_encoding(&encInfo) != e_success)
            return 1;
    }
    else if (operation == e_decode)
    {
        DecodeInfo decInfo = {0};

        if (argc != 3 && argc != 4)
            return 1;

        decInfo.stego_image_fname = argv[2];

        if (argc == 4)
            decInfo.requested_output_fname = argv[3];

        printf("\n========== LSB IMAGE STEGNOGRAPHY ==========\n");
        printf("\n--------------- DECODING COMPLETE -------------------\n\n");

        if (do_decoding(&decInfo) != e_success)
            return 1;
    }
    else
    {
        printf("ERROR: Unsupported operation.\n");
        return 1;
    }

    return 0;
}