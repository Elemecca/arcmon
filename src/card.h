#ifndef ARCMON_CARD_H_
#define ARCMON_CARD_H_

typedef void* card_t;

int card_open (card_t card);
int card_meta (card_t card, char message[], size_t size);
int card_status (card_t card, char message[], size_t size);
void card_free (card_t card);

int cards_find (card_t cards[], size_t max);

#endif
