#include <iostream>

#include "parser.h"

// If parameter is not true, test fails
#define IS_TRUE(x) { if (!x) std::cout << __FUNCTION__ << " failed on line " << __LINE__ << std::endl; }
#define IS_FALSE(x) { if (x) std::cout << __FUNCTION__ << " failed on line " << __LINE__ << std::endl; }



void test_valid()
{
    unsigned int p1, p2_5, p4, p10 = 0;
    unsigned char valid[] = {0x40, 0x0D, 0x04, 0x00, 0x30, 0x00, 0x31, 0x00, 0x32, 0x00, 0x33, 0x00, 0x00, 0x00, 0x00, 0xE9};
    IS_FALSE(parseReadAck(valid, 1, &p1, &p2_5, &p4, &p10));
    IS_TRUE(parseReadAck(valid, 16, &p1, &p2_5, &p4, &p10));
    IS_TRUE(p1 == 48 && p2_5 == 49);

    // Our own data below :O
    unsigned char valid2[] = {0x40, 0xd, 0x4, 0x0, 0x57, 0x0, 0x5c, 0x0, 0x5d, 0x0, 0x60, 0x0, 0x0, 0x0, 0x0, 0x3f};
    IS_TRUE(parseReadAck(valid2, 16, &p1, &p2_5, &p4, &p10));
    std::cout << "p1 " << p1 << " p2.5 " << p2_5 << " p4 " << p4 << std::endl;
}

void test_invalid()
{
    unsigned int p1, p2_5, p4, p10 = 0;
    unsigned char data[] = {0x40, 0xd, 0x4, 0x0, 0x18, 0x0, 0x19, 0x0, 0x19, 0x0, 0x19, 0x0, 0x0, 0xc0, 0x96, 0x96};
    IS_FALSE(parseReadAck(data, 16, &p1, &p2_5, &p4, &p10));

    unsigned char data2[] = {0x40, 0xd, 0x4, 0x0, 0x1c, 0x0, 0x1d, 0x0, 0x1d, 0x0, 0x1d, 0x0, 0x0, 0x0, 0x96, 0x96};
    IS_FALSE(parseReadAck(data2, 16, &p1, &p2_5, &p4, &p10));

}

int main(void) {
    test_valid();
    test_invalid();
}