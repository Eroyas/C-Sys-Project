/*!
 *
 * \file : FIFO_strategy.c
 *
 * \brief : Stratégie sur le temps de résidence d’un bloc.
 * 
 * \author : Yasin EROGLU - Jonathan BOUDAB - Ugo EL KOUBI - Théo DONZELLE
 *
 */

#include "strategy.h"
#include "low_cache.h"
#include "cache_list.h"

// Initialisation de la stratégie FIFO
void *Strategy_Create(struct Cache *pcache) {
    // Allocation et initialisation de la liste FIFO
    return Cache_List_Create();
}

// Suppression de la liste FIFO
void Strategy_Close(struct Cache *pcache) {
    Cache_List_Delete(pcache->pstrategy);
}

// Vide la liste FIFO
void Strategy_Invalidate(struct Cache *pcache) {
    Cache_List_Clear((struct Cache_List*) pcache->pstrategy);
}

/* 
 * On joue ici sur le temps de résidence d’un bloc c'est-à-dire son âge :
 * on remplace le bloc le plus vieux du cache (celui qui y est monté dans 
 * le cache depuis le plus longtemps).
 * 
 * On prend donc le premier bloc de la liste FIFO, le bloc le plus ancien, 
 * et on le passe en fin de liste car il devient du coup le plus récent.
 */
struct Cache_Block_Header *Strategy_Replace_Block(struct Cache *pcache) {
    
    // On ajoute au cache une structure de données supplémentaire, 
    // une liste de pointeurs sur les blocs valides du cache.

    struct Cache_Block_Header *pbh;
    struct Cache_List *fifo_list = (struct Cache_List*) pcache->pstrategy;

     /* On cherche d'abord un bloc invalide */
    if ((pbh = Get_Free_Block(pcache)) != NULL) {
        // On ajoute les blocs invalides dans la liste.
        Cache_List_Append(fifo_list, pbh);
        return pbh;
    }

    // Sinon on prend le premier bloc et le passe en fin de liste.
    pbh = Cache_List_Remove_First(fifo_list);
    Cache_List_Append(fifo_list, pbh);

    return pbh;    
}

 /*!
 * FIFO : Rien à faire ici.
 * Seul l'age de la page compte, pas la fréquence des accès
 */  
void Strategy_Read(struct Cache *pcache, struct Cache_Block_Header *pbh) 
{
}  
  
/*!
 * FIFO : Rien à faire ici.
 * Seul l'age de la page compte, pas la fréquence des accès
 */  
void Strategy_Write(struct Cache *pcache, struct Cache_Block_Header *pbh)
{
} 

char *Strategy_Name()
{
    return "FIFO";
}
