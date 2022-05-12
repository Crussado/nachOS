#include "../userprog/syscall.h"
#include "lib.h"
#include <math.h>

unsigned strlen (const char *s ) {
    unsigned cant = 0;
    for (;*s != '\0'; s++, cant++);
    return cant;
}

void puts (const char *s ) {
    Write(s, (int) strlen(s), CONSOLE_OUTPUT);
}

void itoa (int n , char *str ) {
    int cantCaracteres = 1;
    int pot = 10;
    while(n / pot >= 1) {
        cantCaracteres ++;
        pot *=10;
    }
    char cero = '0';
    int div;
    pot = pow(pot, cantCaracteres - 1);
    for(; pot >= 1; str++) {
        div = n / pot;
        *str = cero + div;
        n %= pot;
        pot /= 10;
    }
    *str = '\0';
}
