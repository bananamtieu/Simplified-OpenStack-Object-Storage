#include <iostream>
#include <string>
#include <cstring>
#include <openssl/evp.h>

using namespace std;

string hex_to_binary(const string& hex_string) {
    string binary_string;
    binary_string.reserve(hex_string.length() * 4); // Reserve space for efficiency

    for (char hex_char : hex_string) {
        switch (hex_char) {
            case '0':
                binary_string += "0000";
                break;
            case '1':
                binary_string += "0001";
                break;
            case '2':
                binary_string += "0010";
                break;
            case '3':
                binary_string += "0011";
                break;
            case '4':
                binary_string += "0100";
                break;
            case '5':
                binary_string += "0101";
                break;
            case '6':
                binary_string += "0110";
                break;
            case '7':
                binary_string += "0111";
                break;
            case '8':
                binary_string += "1000";
                break;
            case '9':
                binary_string += "1001";
                break;
            case 'A':
            case 'a':
                binary_string += "1010";
                break;
            case 'B':
            case 'b':
                binary_string += "1011";
                break;
            case 'C':
            case 'c':
                binary_string += "1100";
                break;
            case 'D':
            case 'd':
                binary_string += "1101";
                break;
            case 'E':
            case 'e':
                binary_string += "1110";
                break;
            case 'F':
            case 'f':
                binary_string += "1111";
                break;
            default:
                cerr << "Invalid hexadecimal digit '" << hex_char << "'" << endl;
                return "";
        }
    }

    return binary_string;
}

string md5_hash(const string& input, int partition) {
    EVP_MD_CTX *mdctx;
    const EVP_MD *md;
    unsigned int md_len;

    OpenSSL_add_all_digests();
    md = EVP_get_digestbyname("md5");

    static unsigned char output[EVP_MAX_MD_SIZE];
    mdctx = EVP_MD_CTX_new();
    EVP_DigestInit_ex(mdctx, md, NULL);
    EVP_DigestUpdate(mdctx, input.c_str(), input.length());
    EVP_DigestFinal_ex(mdctx, output, &md_len);
    EVP_MD_CTX_free(mdctx);

    size_t mdSize = EVP_MD_size(EVP_md5());
    char *hexRep = new char[mdSize * 2 + 1]; // +1 for null terminator

    for (int i = 0; i < mdSize; i++) {
        sprintf(hexRep + 2 * i, "%02x", output[i]);
    }
    string binary_hash = hex_to_binary(hexRep);
    delete[] hexRep;
    int x = binary_hash.length() - partition;
    string hashValue = binary_hash.substr(x);

    return hashValue;
}
