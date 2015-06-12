
#include "cache_list.h"
#include <stdio.h>
#include <stdlib.h>
#include <stdbool.h>
#include <assert.h>

/*! Création d'une liste de blocs */
struct Cache_List *Cache_List_Create(){
	struct Cache_List *racine = malloc ( sizeof (struct Cache_List) );
	if ( racine != NULL ){
		//on boucle la tete de liste sur elle même
		racine->prev = racine;
		racine->next = racine;
		racine->pheader = NULL;
	}

	return racine;
}

/*! Destruction d'une liste de blocs */
void Cache_List_Delete(struct Cache_List *list){

	Cache_List_Clear(list);
	free(list);
}

/*! Insertion d'un élément à la fin */
void Cache_List_Append(struct Cache_List *list, struct Cache_Block_Header *pbh){
    struct Cache_List *pnew = NULL;

    pnew = malloc(sizeof(struct Cache_List));
    pnew->pheader = pbh;

    //On se place en fin de liste
    pnew->prev = list->prev;
    //On fait les "raccords"
    list->prev->next = pnew;
    list->prev = pnew;
    pnew->next = list;
}

/*! Insertion d'un élément au début*/
void Cache_List_Prepend(struct Cache_List *list, struct Cache_Block_Header *pbh){

	struct Cache_List *pnew = NULL;

	pnew = malloc(sizeof(struct Cache_List));
	pnew->pheader = pbh;

	//On se place en tete de liste
	pnew->prev = list;
	//On fait les "raccords"
	list->next->prev = pnew;
	list->next = pnew;
	pnew->next = list->next;
}

/*! Retrait du premier élément */
struct Cache_Block_Header *Cache_List_Remove_First(struct Cache_List *list){
	
	struct Cache_List *pcurr = list->next;
	struct Cache_Block_Header *header = pcurr->pheader;
	if(pcurr == list) return NULL;
	
	pcurr->prev->next = pcurr->next;
	pcurr->next->prev = pcurr->prev;
	
	free(pcurr);
	return header;
}

/*! Retrait du dernier élément */
struct Cache_Block_Header *Cache_List_Remove_Last(struct Cache_List *list){

	struct Cache_List *pcurr = list->prev;
	struct Cache_Block_Header *header = pcurr->pheader;
	if(pcurr == list) return NULL;
	pcurr->next->prev = pcurr->prev;
	pcurr->prev->next = pcurr->next;
	free(pcurr);
	
	return header;
}

/*! Retrait d'un élément quelconque */
struct Cache_Block_Header *Cache_List_Remove(struct Cache_List *list, struct Cache_Block_Header *pbh){

	struct Cache_List *pcurr = NULL;
      	
      	//on parcours la liste
      	for (pcurr = list->next; pcurr != list;  pcurr = pcurr->next){
      		//On cherche l'element
    		if( pcurr->pheader == pbh ){
    			
    			//Si on le trouve on le supprime
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

/*! Remise en l'état de liste vide */
void Cache_List_Clear(struct Cache_List *list){
	//Tant qu'on ne retombe pas sur la tete de liste, on supprime tous les elements
	while(list->next != list){
		Cache_List_Remove_First(list);
	}
}

/*! Test de liste vide */
bool Cache_List_Is_Empty(struct Cache_List *list){
	bool res = false;
	
	if ( list->next == list ){
		res = true;
	}
	
	return res;
}

/*! Transférer un élément à la fin */
void Cache_List_Move_To_End(struct Cache_List *list,struct Cache_Block_Header *pbh){

	struct Cache_Block_Header *pcurr;
	if(list->prev->pheader == pbh){
		return;
	}
	//On supprime l'element recherché dans la liste
	pcurr = Cache_List_Remove(list, pbh);
	if(pcurr == NULL){
		 pcurr = pbh;
	}
	//On l'ajoute en fin de liste
	Cache_List_Append(list, pcurr);

}

/*! Transférer un élément  au début */
void Cache_List_Move_To_Begin(struct Cache_List *list,struct Cache_Block_Header *pbh){
	
	struct Cache_Block_Header *pcurr;
	if(list->next->pheader == pbh){
		return;
	}
	//On supprime l'element recherché dans la liste
	pcurr = Cache_List_Remove(list, pbh);
	if(pcurr == NULL){
		 pcurr = pbh;
	}
	//On l'ajoute en debut de liste
	Cache_List_Prepend(list, pcurr);
}

