#include <stdio.h>
#include <stdarg.h>

#include "common.h"
#include "card.h"

#define CARDS_MAX 16
#define MESSAGE_MAX 4096


void errlog (const char *format, ...) {
    va_list args;
    va_start( args, format );
    vfprintf( stderr, format, args );
    va_end( args );

    fprintf( stderr, "\n" );
}


int main (int argc, char *argv[]) {
    card_t cards[CARDS_MAX + 1];
    char message[MESSAGE_MAX];
    int idx, written;

    setvbuf( stdout, NULL, _IOLBF, 0 );

    if (-1 == cards_find( cards, CARDS_MAX )) {
        return 2;
    }

    for (idx = 0; 0 != cards[idx]; idx++) {
        if (!card_open( cards[idx] ))
            return 3;

        written = card_meta( cards[idx], message, MESSAGE_MAX );
        if (written < 0) return 3;
        printf( "%s", message );
    }

    while (1) {
        for (idx = 0; 0 != cards[idx]; idx++) {
            written = card_status( cards[idx], message, MESSAGE_MAX );
            if (written < 0) return 3;
            printf( "%s", message );
        }

        sleep( 5 );
    }
}
