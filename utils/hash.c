/*
 * Copyright (C) 2014 Johannes Schauer <j.schauer@email.de>
 *
 * This program is free software; you can redistribute it and/or modify it
 * under the terms of the GNU General Public License version 2, as published by
 * the Free Software Foundation.
 *
 * This program is distributed in the hope that it will be useful, but WITHOUT
 * ANY WARRANTY; without even the implied warranty of MERCHANTABILITY or
 * FITNESS FOR A PARTICULAR PURPOSE. See the GNU General Public License for
 * more details.
 *
 * You should have received a copy of the GNU General Public License along with
 * this program; if not, write to the Free Software Foundation, Inc., 51
 * Franklin Street, Fifth Floor, Boston, MA  02110-1301, USA.
 *
 */

#include <stdlib.h>
#include <stdio.h>
#include <openssl/sha.h>
#include <openssl/md5.h>
#include <stddef.h>

#define bufsize 32768

/*
 * we use this table to convert from a base36 char (ignoring case) to an
 * integer or from a hex string to binary (in the latter case letters g-z and
 * G-Z remain unused)
 * we "waste" these 128 bytes of memory so that we don't need branching
 * instructions when decoding
 * we only need 128 bytes because the input is a *signed* char
 */
static unsigned char base36_decoding_table[] = {
/* 0x00 */ 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0,
/* 0x10 */ 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0,
/* 0x20 */ 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0,
/* 0x30 */ 0, 1, 2, 3, 4, 5, 6, 7, 8, 9, 0, 0, 0, 0, 0, 0,
/* 0x40 */ 0, 10, 11, 12, 13, 14, 15, 16, 17, 18, 19, 20, 21, 22, 23, 24,
/* 0x50 */ 25, 26, 27, 28, 29, 30, 31, 32, 33, 34, 35, 0, 0, 0, 0, 0,
/* 0x60 */ 0, 10, 11, 12, 13, 14, 15, 16, 17, 18, 19, 20, 21, 22, 23, 24,
/* 0x70 */ 25, 26, 27, 28, 29, 30, 31, 32, 33, 34, 35, 0, 0, 0, 0, 0
};

/*
 * table to convert from a byte into the two hexadecimal digits representing
 * it
 */
static char     base16_encoding_table[][2] = {
    "00", "01", "02", "03", "04", "05", "06", "07", "08", "09", "0A", "0B",
    "0C", "0D", "0E", "0F", "10", "11", "12", "13", "14", "15", "16", "17",
    "18", "19", "1A", "1B", "1C", "1D", "1E", "1F", "20", "21", "22", "23",
    "24", "25", "26", "27", "28", "29", "2A", "2B", "2C", "2D", "2E", "2F",
    "30", "31", "32", "33", "34", "35", "36", "37", "38", "39", "3A", "3B",
    "3C", "3D", "3E", "3F", "40", "41", "42", "43", "44", "45", "46", "47",
    "48", "49", "4A", "4B", "4C", "4D", "4E", "4F", "50", "51", "52", "53",
    "54", "55", "56", "57", "58", "59", "5A", "5B", "5C", "5D", "5E", "5F",
    "60", "61", "62", "63", "64", "65", "66", "67", "68", "69", "6A", "6B",
    "6C", "6D", "6E", "6F", "70", "71", "72", "73", "74", "75", "76", "77",
    "78", "79", "7A", "7B", "7C", "7D", "7E", "7F", "80", "81", "82", "83",
    "84", "85", "86", "87", "88", "89", "8A", "8B", "8C", "8D", "8E", "8F",
    "90", "91", "92", "93", "94", "95", "96", "97", "98", "99", "9A", "9B",
    "9C", "9D", "9E", "9F", "A0", "A1", "A2", "A3", "A4", "A5", "A6", "A7",
    "A8", "A9", "AA", "AB", "AC", "AD", "AE", "AF", "B0", "B1", "B2", "B3",
    "B4", "B5", "B6", "B7", "B8", "B9", "BA", "BB", "BC", "BD", "BE", "BF",
    "C0", "C1", "C2", "C3", "C4", "C5", "C6", "C7", "C8", "C9", "CA", "CB",
    "CC", "CD", "CE", "CF", "D0", "D1", "D2", "D3", "D4", "D5", "D6", "D7",
    "D8", "D9", "DA", "DB", "DC", "DD", "DE", "DF", "E0", "E1", "E2", "E3",
    "E4", "E5", "E6", "E7", "E8", "E9", "EA", "EB", "EC", "ED", "EE", "EF",
    "F0", "F1", "F2", "F3", "F4", "F5", "F6", "F7", "F8", "F9", "FA", "FB",
    "FC", "FD", "FE", "FF"
};

int calc_md5(FILE * file, unsigned char *hash)
{
    int             bytesRead;
    char           *buffer;
    MD5_CTX         md5;

    MD5_Init(&md5);
    buffer = malloc(bufsize);
    if (buffer == NULL) {
        return -1;
    }
    while ((bytesRead = fread(buffer, 1, bufsize, file))) {
        MD5_Update(&md5, buffer, bytesRead);
    }
    MD5_Final(hash, &md5);
    free(buffer);
    return 0;
}

int calc_sha256(FILE * file, unsigned char *hash)
{
    int             bytesRead;
    char           *buffer;
    SHA256_CTX      sha256;

    SHA256_Init(&sha256);
    buffer = malloc(bufsize);
    if (buffer == NULL) {
        return -1;
    }
    while ((bytesRead = fread(buffer, 1, bufsize, file))) {
        SHA256_Update(&sha256, buffer, bytesRead);
    }
    SHA256_Final(hash, &sha256);
    free(buffer);
    return 0;
}

/* decodes a zero terminated string containing hex characters into their
 * binary representation. The length of the string must be even as pairs of
 * characters are converted to one output byte. The output buffer must be at
 * least half the length of the input string.
 */
void hex2binary(const char *hex, unsigned char *binary)
{
    unsigned char   val1,
                    val2;
    const char     *c1,
                   *c2;
    unsigned char  *b;

    for (b = binary, c1 = hex, c2 = hex + 1;
         *c1 != '\0' && *c2 != '\0'; b++, c1 += 2, c2 += 2) {
        val1 = base36_decoding_table[(int)(*c1)];
        val2 = base36_decoding_table[(int)(*c2)];
        *b = (val1 << 4) | val2;
    }
}

char           *binary2hex(const unsigned char *binary, size_t length)
{
    char           *out;
    char           *p;
    size_t          i;

    out = malloc(length * 2 + 1);
    if (out == NULL) {
        fprintf(stderr, "cannot allocate memory\n");
        return NULL;
    }
    for (i = 0; i < length; i++) {
        p = base16_encoding_table[binary[i]];
        out[i * 2] = p[0];
        out[i * 2 + 1] = p[1];
    }
    out[length * 2] = '\0';
    return out;
}

/*
 * a function to convert a char* of the key into a hash of its first three
 * characters, treating those first three characters as if they represented a
 * number in base36
 *
 * in the future this could be made more dynamic by using the ability of
 * strtoll to convert numbers of base36 and then only retrieving the desired
 * amount of high-bits for the desired size of the hashtable
 */
int base36_decode_triplet(const char *key)
{
    return base36_decoding_table[(int)(key)[0]] * 36 * 36
        + base36_decoding_table[(int)(key)[1]] * 36
        + base36_decoding_table[(int)(key)[2]];
}
