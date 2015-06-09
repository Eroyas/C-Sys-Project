#include "cache_list.h"
#include <stdio.h>
#include <stdlib.h>
#include <stdbool.h>

struct Cache_List *Cache_List_Create(){
	
	struct Cache_List *racine = malloc ( sizeof *racine );
	if ( racine != NULL ){
		racine->prev = racine;
		racine->next = racine;
	}
	return racine;
}

void Cache_List_Delete(struct Cache_List *list){

	assert(list != NULL);
    	struct Cache_List *pcurr = NULL;
    	
    	if ( pcurr != list->next ) {
    		struct Cache_List *plast = pcurr->prev;
      		struct Cache_List *pnext = NULL;
      		
      		for( ; pcurr != list->next; pcurr = pnext){
		    pnext = pcurr->next;
		    free(pcurr);
		}
		
		plast->next = pcurr;
		pcurr->prev = plast;
	}	
}

void Cache_List_Append(struct Cache_List *list, struct Cache_Block_Header *pbh){

    struct Cache_List *pcurr = NULL;
    struct Cache_List *pnew = NULL;

    pnew = malloc(sizeof(struct Cache_List));
    pnew->pheader = pbh;

    for (pcurr = list->next; pcurr != list->next;  pcurr = pcurr->next)
    { }

    pnew->prev = pcurr->prev;
    pcurr->prev->next = pnew;
    pcurr->prev = pnew;
    pnew->next = pcurr;
}

void Cache_List_Prepend(struct Cache_List *list, struct Cache_Block_Header *pbh){

	struct Cache_List *pcurr = NULL;
	struct Cache_List *pnew = NULL;

	pnew = malloc(sizeof(struct Cache_List));
	pnew->pheader = pbh;

	pnew->prev = pcurr->prev;
	pcurr->prev->next = pnew;
	pcurr->prev = pnew;
	pnew->next = pcurr;
}

struct Cache_Block_Header *Cache_List_Remove_First(struct Cache_List *list){
	
	struct Cache_List *pcurr = NULL;
	
	pcurr = list->next;
	struct Cache_List *plast = pcurr->prev;
	free(pcurr);

	plast->next = pcurr;
	pcurr->prev = plast;
}

struct Cache_Block_Header *Cache_List_Remove_Last(struct Cache_List *list){

	struct Cache_List *pcurr = NULL;
      	
      	for (pcurr = list->next; pcurr != list->next;  pcurr = pcurr->next)
    { }
	struct Cache_List *plast = pcurr->prev;
	free(pcurr);
	
	plast->next = pcurr;
	pcurr->prev = plast;
}

struct Cache_Block_Header *Cache_List_Remove(struct Cache_List *list, struct Cache_Block_Header *pbh){

	struct Cache_List *pcurr = NULL;
      	
      	for (pcurr = list->next; pcurr != list->next;  pcurr = pcurr->next)
    { }
	struct Cache_List *plast = pcurr->prev;
	free(pcurr);
	
	plast->next = pcurr;
	pcurr->prev = plast;
}
