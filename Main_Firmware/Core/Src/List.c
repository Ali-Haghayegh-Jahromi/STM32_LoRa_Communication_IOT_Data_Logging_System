#include "List.h"

dict_t **dictAlloc(void) {
    return malloc(sizeof(dict_t));
}

void dict_dealloc(dict_t *dict) {
    dict_t *ptr;
    for (ptr = dict; ptr != NULL; ptr = ptr->next) {
        free(ptr);
    }
}

uint8_t dictGetItem(dict_t *dict, char *key) {
    dict_t *ptr;
    for (ptr = dict; ptr != NULL; ptr = ptr->next) {
        if (strcmp(ptr->key, key) == 0) {
            return ptr->value;
        }
    }
    
    return NULL;
}

void dictDelItem(dict_t **dict, char *key) 
{
    dict_t *ptr, *prev;
    for (ptr = *dict, prev = NULL; ptr != NULL; prev = ptr, ptr = ptr->next) {
        if (strcmp(ptr->key, key) == 0) 
				{
            if (ptr->next != NULL) 
						{
                if (prev == NULL) 
								{
                    *dict = ptr->next;
                } 
								else 
								{
                    prev->next = ptr->next;
                }
            } 
						else if (prev != NULL) 
						{
                prev->next = NULL;
            } 
						else 
						{
                *dict = NULL;
            }
            
            free(ptr->key);
            free(ptr);
            
            return;
        }
    }
}

int dictSize(dict_t *dict) 
{
    int size = 0;
    
    dict_t *ptr;
    for (ptr = dict; ptr != NULL; ptr = ptr->next) 
		{
        size++;
    }
    
    return size;
}

void dictAddItem(dict_t **dict, char *key, uint8_t value) 
{
    dictDelItem(dict, key); /* If we already have a item with this key, delete it. */
    dict_t *d = malloc(sizeof(struct dict_t_struct));
    d->key = malloc(strlen(key)+1);
    strcpy(d->key, key);
    d->value = value;
    d->next = *dict;
    *dict = d;
}
