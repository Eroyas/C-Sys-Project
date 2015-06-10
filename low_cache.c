/*!
 *
 * \file : low_cache.c
 *
 * \brief : Structures de données pour l'implémentation du cache.
 * 
 * \author : Yasin EROGLU - Jonathan BOUDAB - Ugo EL KOUBI - Théo DONZELLE
 *
*/

#include "low_cache.h"
 
/*Retourne le premier bloc libre du cache ou le pointeur NULL si le cache est plein. Cette
fonction doit être invoquée par la stratégie de remplacement avant de considérer
l’utilisation d’un bloc valide.*/
struct Cache_Block_Header *Get_Free_Block(struct Cache *pcache)
{
	unsigned int i;
	// on parcourt tous les blocs du cache
	for(i = 0;i<pcache->nblocks;i++) {
		// si il n'est pas valide 
		if ((pcache->headers[i].flags & VALID) != 0x1)
			return &(pcache->headers[i]);
	}
	// le cache est plein
	return NULL;
}