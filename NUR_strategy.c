/*!
 *
 * \file : NUR_strategy.c
 *
 * \brief : Stratégie sur le bloc qui n’a pas (de préférence, pas du tout) été utilisé récemment.
 * 
 * \author : Yasin EROGLU - Jonathan BOUDAB - Ugo EL KOUBI - Théo DONZELLE
 *
 */

#include "strategy.h"
#include "low_cache.h"

 // Flag pour le bit de référence
#define REFER 0x4

// NDEREF est la période avec laquelle le bit R est remis à 0
#define NDEREF 100


 /* Création d'une structure avec cptderf : le compteur de déférençage et 
 nderef : période entre deux dérérençages*/
struct NUR_Strategy
{
    unsigned nderef;    /* Période de déréférençage */
    unsigned cptderef;  /* compteur */
};

 // Déréférencage
static void Dereferencage(struct Cache *pcache)
{
    struct NUR_Strategy *pstrat = (struct NUR_Strategy *)(pcache)->pstrategy;
    int ib;

    // Déférencement tous les deref accès
    if (--pstrat->cptderef > 0){
        return;
    }
    // Remise à zero de tous les bits de déférence
    for (ib = 0; ib < pcache->nblocks; ib++){
        pcache->headers[ib].flags &= ~REFER;
    }
    // Remise a 0 du compteur
    pstrat->cptderef = pstrat->nderef;
    // Incrémentation du compteur de déréférençage
    ++pcache->instrument.n_deref;
}



 // Initialisation de la stratégie.
void *Strategy_Create(struct Cache *pcache) 
{
    struct NUR_Strategy *pstrat = malloc(sizeof((struct NUR_Strategy *) pcache->pstrategy));
    pstrat->nderef = pcache->nderef;
    pstrat->cptderef = pcache->nderef;
    return pstrat;
}

// Calcul du nombre RM (RM=2*R+M)
int calculateRM(int r, int m){
    int rm = ((r) ? 1 : 0) << 1;
    rm |= (m) ? 1 : 0;
    return rm;
}

// Fermeture de la stratégie.
void Strategy_Close(struct Cache *pcache)
{
    free(pcache->pstrategy);
}

// Invalidation du cache
void Strategy_Invalidate(struct Cache *pcache) 
{
    struct NUR_Strategy *pstrat = (struct NUR_Strategy *)(pcache)->pstrategy;
    // Si nderef est nul on ne déférence jamais
    if (pstrat->nderef != 0) {
        pstrat->cptderef = 1;
        Dereferencage(pcache);
    }   
}

 /* On cherche un bloc invilade, si il n'y en a pas pas on prend le bloc
 donc le nombre RM calculé est le plus petit
 */
struct Cache_Block_Header *Strategy_Replace_Block(struct Cache *pcache) 
{
    int i, rm;
    int min = 10;

    struct Cache_Block_Header *best_pbh = NULL;
    struct  Cache_Block_Header *pbh;
   // Recherche d'un bloc invalide
    if ((pbh = Get_Free_Block(pcache)) != NULL) return pbh;
    // Recherche du bloc avec le nombre RM le minimum
    for (i = 0; i < pcache->nblocks; i++)
    {
        pbh = &pcache->headers[i];
        // Construction du nombre RM avec le bit de référence et le bit de modification (RM = 2*R+M)
        rm = calculateRM(pbh->flags & REFER, pbh->flags & MODIF);
        if (rm < min) {
            min = rm;
            best_pbh = pbh;
        }   
    }
    return best_pbh;    
}


 // Si une lecture ou une ecriture est faite, on déréférence si il y a besoin et on met le bit R à 1
void Strategy_Read(struct Cache *pcache, struct Cache_Block_Header *pbh) 
{
    Dereferencage(pcache);
    pbh->flags |= REFER;
} 
  
void Strategy_Write(struct Cache *pcache, struct Cache_Block_Header *pbh)
{
    Dereferencage(pcache);
    pbh->flags |= REFER;
} 

//! Identification de la stratégie.
char *Strategy_Name()
{
    return "NUR";
}



