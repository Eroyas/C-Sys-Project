
#include "cache_list.h"
#include <stdio.h>
#include <stdlib.h>
#include <stdbool.h>
#include <assert.h>


struct Cache_List *Cache_List_Create(){
	struct Cache_List *racine = malloc ( sizeof (struct Cache_List) );
	if ( racine != NULL ){
		racine->prev = racine;
		racine->next = racine;
		racine->pheader = NULL;
	}

	return racine;
}

void Cache_List_Delete(struct Cache_List *list){

	Cache_List_Clear(list);
	free(list);
}

void Cache_List_Append(struct Cache_List *list, struct Cache_Block_Header *pbh){
    struct Cache_List *pnew = NULL;

    pnew = malloc(sizeof(struct Cache_List));
    pnew->pheader = pbh;

    pnew->prev = list->prev;
    list->prev->next = pnew;
    list->prev = pnew;
    pnew->next = list;
}

void Cache_List_Prepend(struct Cache_List *list, struct Cache_Block_Header *pbh){

	struct Cache_List *pnew = NULL;

	pnew = malloc(sizeof(struct Cache_List));
	pnew->pheader = pbh;

	pnew->prev = list;
	list->next->prev = pnew;
	list->next = pnew;
	pnew->next = list->next;
}

struct Cache_Block_Header *Cache_List_Remove_First(struct Cache_List *list){
	
	struct Cache_List *pcurr = list->next;
	struct Cache_Block_Header *header = pcurr->pheader;
	if(pcurr == list) return NULL;
	pcurr->prev->next = pcurr->next;
	pcurr->next->prev = pcurr->prev;
	
	free(pcurr);
	return header;
}

struct Cache_Block_Header *Cache_List_Remove_Last(struct Cache_List *list){
	struct Cache_List *pcurr = list->prev;
	struct Cache_Block_Header *header = pcurr->pheader;
	if(pcurr == list) return NULL;
	pcurr->next->prev = pcurr->prev;
	pcurr->prev->next = pcurr->next;
	free(pcurr);
	
	return header;
}

struct Cache_Block_Header *Cache_List_Remove(struct Cache_List *list, struct Cache_Block_Header *pbh){

	struct Cache_List *pcurr = NULL;
      	
      	for (pcurr = list->next; pcurr != list;  pcurr = pcurr->next){
    		if( pcurr->pheader == pbh ){
    			struct Cache_Block_Header *header = pcurr->pheader;
    			struct Cache_List *plast = pcurr->prev;
				plast->next = pcurr->next;
				pcurr->next->prev = plast;
				free(pcurr);
				return header;
	    	}			
	}
	return NULL;
}

void Cache_List_Clear(struct Cache_List *list){
	while(list->next != list){
		Cache_List_Remove_First(list);
	}
}

bool Cache_List_Is_Empty(struct Cache_List *list){
	bool res = false;
	
	if ( list->next == list ){
		res = true;
	}
	
	return res;
}

void Cache_List_Move_To_End(struct Cache_List *list,struct Cache_Block_Header *pbh){

	struct Cache_Block_Header *pcurr;
	if(list->prev->pheader == pbh){
		return;
	}
	pcurr = Cache_List_Remove(list, pbh);
	if(pcurr == NULL){
		 pcurr = pbh;
	}
	Cache_List_Append(list, pcurr);

}

void Cache_List_Move_To_Begin(struct Cache_List *list,struct Cache_Block_Header *pbh){
	
	struct Cache_Block_Header *pcurr;
	if(list->next->pheader == pbh){
		return;
	}
	pcurr = Cache_List_Remove(list, pbh);
	if(pcurr == NULL){
		 pcurr = pbh;
	}
	Cache_List_Prepend(list, pcurr);
}

