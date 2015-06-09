/*!
 *
 * \file : LRU_strategy.c
 *
 * \brief : Stratégie sur le bloc qui a été le moins récemment accédé.
 * 
 * \author : Yasin EROGLU - Jonathan BOUDAB - Ugo EL KOUBI - Théo DONZELLE
 *
 */

#include "strategy.h"
#include "low_cache.h"
#include "cache_list.h"

// Initialisation de la stratégie LRU
void *Strategy_Create(struct Cache *pcache) {
    // Allocation et initialisation de la liste LRU
    return Cache_List_Create();
}

// Suppression de la liste LRU
void Strategy_Close(struct Cache *pcache) {
    Cache_List_Delete(pcache->pstrategy);
}

// Vide la liste LRU
void Strategy_Invalidate(struct Cache *pcache) {
    Cache_List_Clear((struct Cache_List*) pcache->pstrategy);
}

/* 
 * On remplace ici le bloc qui a été le moins récemment accédé.
 * Chaque fois qu’un bloc change d’affectation, on le transfère en queue
 * de liste. Mais on effectue également ce transfert vers la queue chaque 
 * fois qu’un bloc est utilisé, c’est-à-dire chaque fois qu’il est atteint 
 * par une opération de lecture ou d’écriture. Ainsi le bloc en tête de la 
 * liste lru est le bloc le moins récemment utilisé, et c’est lui qui 
 * sera remplacé.
 * 
 * On prend donc le premier bloc de la liste LRU, le bloc le plus ancien,
 * et on le passe en fin de liste car il devient du coup le plus récent. 
 */
struct Cache_Block_Header *Strategy_Replace_Block(struct Cache *pcache) {
    
    // On ajoute au cache une structure de données supplémentaire, 
    // une liste de pointeurs sur les blocs valides du cache.

    struct Cache_Block_Header *pbh;
    struct Cache_List *lru_list = (struct Cache_List*) pcache->pstrategy;

    /* On cherche d'abord un bloc invalide */
    if ((pbh = Get_Free_Block(pcache)) != NULL) {
        // On ajoute les blocs invalides dans la liste.
        Cache_List_Append(lru_list, pbh);  
        return pbh;
    }

    // Sinon on prend le premier bloc et le passe en fin de liste.
    pbh = Cache_List_Remove_First(lru_list);
    Cache_List_Append(lru_list, pbh);

    return pbh;    
}

 /*!
 * LRU : La fréquence des accès compte, le bloc référencé devient le bloc le 
 * plus recent et passe en fin de liste.
 */  
void Strategy_Read(struct Cache *pcache, struct Cache_Block_Header *pbh) {
    Cache_List_Move_To_End((struct Cache_List*) pcache->pstrategy, pbh);
}  

 /*!
 * LRU : La fréquence des accès compte, le bloc référencé devient le bloc le 
 * plus recent et passe en fin de liste.
 */ 
void Strategy_Write(struct Cache *pcache, struct Cache_Block_Header *pbh) {
    Cache_List_Move_To_End((struct Cache_List*) pcache->pstrategy, pbh);
} 

char *Strategy_Name()
{
    return "LRU";
}
