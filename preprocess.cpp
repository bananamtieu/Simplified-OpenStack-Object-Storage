#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <openssl/evp.h>
#include "preprocess.h"

void hex_to_binary(const char *hex_string, char *binary_string) {
    int i, j;
    for (i = 0, j = 0; hex_string[i] != '\0'; i++, j += 4) {
        switch (hex_string[i]) {
            case '0':
                strcat(binary_string + j, "0000");
                break;
            case '1':
                strcat(binary_string + j, "0001");
                break;
            case '2':
                strcat(binary_string + j, "0010");
                break;
            case '3':
                strcat(binary_string + j, "0011");
                break;
            case '4':
                strcat(binary_string + j, "0100");
                break;
            case '5':
                strcat(binary_string + j, "0101");
                break;
            case '6':
                strcat(binary_string + j, "0110");
                break;
            case '7':
                strcat(binary_string + j, "0111");
                break;
            case '8':
                strcat(binary_string + j, "1000");
                break;
            case '9':
                strcat(binary_string + j, "1001");
                break;
            case 'A':
            case 'a':
                strcat(binary_string + j, "1010");
                break;
            case 'B':
            case 'b':
                strcat(binary_string + j, "1011");
                break;
            case 'C':
            case 'c':
                strcat(binary_string + j, "1100");
                break;
            case 'D':
            case 'd':
                strcat(binary_string + j, "1101");
                break;
            case 'E':
            case 'e':
                strcat(binary_string + j, "1110");
                break;
            case 'F':
            case 'f':
                strcat(binary_string + j, "1111");
                break;
            default:
                printf("Invalid hexadecimal digit '%c'\n", hex_string[i]);
                return;
        }
    }
}

char *md5_hash(const char *input) {
    EVP_MD_CTX *mdctx;
    const EVP_MD *md;
    unsigned int md_len;

    OpenSSL_add_all_digests();
    md = EVP_get_digestbyname("md5");

    static unsigned char output[EVP_MAX_MD_SIZE];
    mdctx = EVP_MD_CTX_new();
    EVP_DigestInit_ex(mdctx, md, NULL);
    EVP_DigestUpdate(mdctx, input, strlen(input));
    EVP_DigestFinal_ex(mdctx, output, &md_len);
    EVP_MD_CTX_free(mdctx);

    size_t mdSize = EVP_MD_size(EVP_md5());

    char *hexRep = (char *)malloc(mdSize * 2 + 1); // +1 for null terminator
    if (hexRep == NULL) {
        // Handle allocation failure
        return NULL;
    }
    char *binRep = (char *)malloc(mdSize * 8 + 1); // +1 for null terminator
    if (binRep == NULL) {
        // Handle allocation failure
        free(hexRep);
        return NULL;
    }
    
    for (int i = 0; i < mdSize; i++) {
        sprintf(hexRep + 2 * i, "%02x", output[i]);
    }
    hex_to_binary(hexRep, binRep);

    free(hexRep);
    return binRep;
}