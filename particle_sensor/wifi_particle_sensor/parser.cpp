#include "parser.h"

bool parseReadAck(unsigned char* data, int size, unsigned int* p1, unsigned int* p2_5, unsigned int* p4, unsigned int* p10) {
    // Data should usually be 16 bytes.
    // Correct: 40 0D 04 00 30 00 31 00 32 00 33 00 00 00 00 E9
    //          [] [] [] [ 1 ] [ 2 ] [ 4 ] [ 10] [] [] [] [ C ]
    if (size != 16) {
        return false;
    }

    int i = 0;
    if (data[i] != '\x40') {
        return false;
    }
    i++;

    int len = data[i];
    i++;

    if (data[i] != '\x04') {
        return false;
    }
    i++;

    // Check checksum.
    unsigned int sum = 0;
    for (int j = 0; j < size-2; j++) {
        sum += data[j];
    }
    unsigned int checksum = (65536 - sum) % 256;
    unsigned int packetChecksum = data[size - 2] * 256 + data[size - 1];
    if (checksum != packetChecksum) {
        return false;
    }

    *p1 = data[i] * 256 + data[i+1];
    i += 2;
    *p2_5 = data[i] * 256 + data[i+1];
    i += 2;
    *p4 = data[i] * 256 + data[i+1];
    i += 2;
    *p10 = data[i] * 256 + data[i+1];

    return true;
}

