#include <stdio.h>
#include <string.h>
#include <stdlib.h>

// ~ 5
/**
 * % == %25 == 37
 * 37   /  2     1
 * 18   /  2     1
 * 13   /  2     1
 * 6    /  2     0
 * 3    /  2     1
 * 1    /  2     1
 * 
 * 15   /  2     1     
 * 7    /  2     1
 * 4    /  2     0
 * 2    /  2     0
 * 1    /  2     1    
 * 100101 >> 4 == 000011(2)
 * 100101 & 15 
 * 001111
 * 
 * 000101 (5)
 * 37 >> 4 == 2
 * 37 & 15 == 5
 * which evaluates to %25 in URL: encoding 
 * 
*/

// Calculate the total number of bits
size_t bs64encode_size(size_t len) {

    size_t ret;
    ret = len;
    if(len % 3 != 0)
        ret += 3 - (len % 3);
    ret /= 3;
    ret *= 4;

    return ret;
}

char *encode_base64(unsigned char *str, size_t str_len) {

    if(str == NULL || str_len == 0)
        return NULL;
    
    const char *bs64chars = "ABCDEFGHIJKLMNOPQRSTUVWXYZabcdefghijklmnopqrstuvwxyz0123456789+/";
    size_t encode_len = bs64encode_size(str_len);
    char *encoded = (char *) malloc(sizeof(char ) * encode_len + 1);
    size_t val, i, j, count = 0, index = 0, tmp, no_of_bits = 0, padding = 0, pos = 0;

    for(i = 0; i  < str_len; i += 3) {

        val = 0, count = 0, no_of_bits = 0;
        for(j = i; j < str_len && j <= i + 2; j++) {
            val = val << 8;
            val = val | str[j];
            count++;
        }

        no_of_bits = count * 8;
        padding = no_of_bits % 3;

        while(no_of_bits != 0) {
            
            if(no_of_bits >= 6) {
                tmp = no_of_bits - 6;

                index = (val >> tmp) & 0x3F;
                no_of_bits -= 6;

            } else {
                tmp = 6 - no_of_bits;
                index = (val << tmp) & 0x3F;
                no_of_bits = 0;
            }
            encoded[pos++] = bs64chars[index];
        }
    }

    for(i = 1; i <= padding; i++) {
        encoded[pos++] = '=';
    }
    encoded[pos] = '\0';

    return encoded; 
}


// char *url_decode(char *encoded, size_t encoded_len) {

//     char *decoded = (char*) malloc(sizeof(char) * encoded_len + 1);
//     const char *hex = "0123456789ABCDEF";
//     int pos = 0, i;

//     for(i = 0; i < encoded_len; i++) {
//         if(encoded[i] == '%'){

//             decoded[pos++] = hex[encoded[i] << 4];
//             i += 3;
//         } else {
//             decoded[pos++] = encoded;
//         }
//     }
// }
int main() {

    char *input_str = "zayyad";
    printf("%s\n", encode_base64(input_str, strlen(input_str)));

    /**
     * For each 3 char, recv 4 char
    */
    return 0;
}

