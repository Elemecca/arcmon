#ifndef ARCMON_CARD_H_
#define ARCMON_CARD_H_

typedef void* card_t;

card_t card_open (int index);
int card_meta (card_t card, char message[], size_t size);
int card_status (card_t card, char message[], size_t size);
void card_free (card_t card);

#endif
