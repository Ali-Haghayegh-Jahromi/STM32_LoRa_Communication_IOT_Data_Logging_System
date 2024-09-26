#include <String.h>
#include <Stdio.h>
#include <Stdint.h>
#include <Stdlib.h>

typedef struct dict_t_struct 
{
    char *key;
    uint8_t value;
		struct dict_t_struct *next;
} dict_t;

dict_t **dictAlloc(void);

void dictDeAlloc(dict_t *dict);

uint8_t dictGetItem(dict_t *dict, char *key);

void dictDelItem(dict_t **dict, char *key);

int dictSize(dict_t *dict);

void dictAddItem(dict_t **dict, char *key, uint8_t value) ;
