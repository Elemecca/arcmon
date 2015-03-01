#include <stdio.h>
#include <stdarg.h>

#include "common.h"
#include "card.h"

#define MESSAGE_MAX 4096


void errlog (const char *format, ...) {
    va_list args;
    va_start( args, format );
    vfprintf( stderr, format, args );
    va_end( args );

    fprintf( stderr, "\n" );
}


int main (int argc, char *argv[]) {
    card_t card;
    char message[MESSAGE_MAX];
    int written;

    setvbuf( stdout, NULL, _IOLBF, 0 );

    card = card_open( 2 );
    if (!card) return 2;

    written = card_meta( card, message, MESSAGE_MAX );
    if (written < 0) return 3;
    printf( "%s", message );

    while (1) {
        written = card_status( card, message, MESSAGE_MAX );
        if (written < 0) return 3;
        printf( "%s", message );

        sleep( 5 );
    }
}
