#include <stdio.h>

const int i = 1;
#define isLittleEndian() (*(char*)&i)

int main(void) {
    printf("Your system uses %s endian byte order!\n",isLittleEndian()?"little":"big");
}
