/*!
 *
 * \file : LRU_strategy.c
 *
 * \brief : Strat�gie sur le bloc qui a �t� le moins r�cemment acc�d�.
 * 
 * \author : Yasin EROGLU - Jonathan BOUDAB - Ugo EL KOUBI - Th�o DONZELLE
 *
 */

#include "strategy.h"
#include "low_cache.h"
#include "cache_list.h"

/* Transforme le pointeur sur la strat�gie en un pointeur sur la liste LRU */
#define LRU_LIST(pcache) ((struct Cache_List *)((pcache)->pstrategy))

/* ------------------------------------------------------------------
 * Fonctions externes (utlis�es par cache.c)
 * ------------------------------------------------------------------
 */

/* Initialisation de la strat�gie 
 * ------------------------------ 
 * On alloue et initialise la liste LRU.
*/
void *Strategy_Create(struct Cache *pcache) 
{
    /* Cr�ation et initialisation de la liste LRU */
    return Cache_List_Create();
}

/* Fermeture de la strat�gie
 * -------------------------
 */
void Strategy_Close(struct Cache *pcache)
{
    Cache_List_Delete(LRU_LIST(pcache));
}

/* Invalidation du cache
 * ---------------------
 * On vide la liste LRU.
 */
void Strategy_Invalidate(struct Cache *pcache)
{
    Cache_List_Clear(LRU_LIST(pcache));
}

/* Algorithme de remplacement de bloc
 * ---------------------------------- 

 * On prend le premier bloc invalide. S'il n'y en a plus on prend le premier
 * bloc de la liste LRU c'est-�-dire le bloc le plus ancien. Comme ce bloc
 * devient le plus r�cent, on le passe en fin de liste.
 */
struct Cache_Block_Header *Strategy_Replace_Block(struct Cache *pcache) 
{
    struct Cache_Block_Header *pbh;
    struct Cache_List *lru_list = LRU_LIST(pcache);

    /* On cherche d'abord un bloc invalide */
    if ((pbh = Get_Free_Block(pcache)) != NULL)
    {
        /* Les blocs invalides ne sont pas dans la liste ; mettons-le en queue */
        Cache_List_Append(lru_list, pbh);  
        return pbh;
    }

    /* Sinon on prend le premier bloc de la liste LRU et on le d�place � la fin
     * de la liste */
    pbh = Cache_List_Remove_First(lru_list);
    Cache_List_Append(lru_list, pbh);

    return pbh;    
}

/* Fonctions "r�flexes" en cas de lecture et d'�criture
 * ---------------------------------------------------- 
 * Le bloc r�f�renc� devient le bloc le plus r�cent et passe donc en queue de
 * la liste LRU.
 */
void Strategy_Read(struct Cache *pcache, struct Cache_Block_Header *pbh) 
{
    Cache_List_Move_To_End(LRU_LIST(pcache), pbh);
}  
  
void Strategy_Write(struct Cache *pcache, struct Cache_Block_Header *pbh)
{
    Cache_List_Move_To_End(LRU_LIST(pcache), pbh);
} 

/* Identification de la strat�gie
 * ------------------------------
 */
char *Strategy_Name()
{
    return "LRU";
}
