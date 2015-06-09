/*!
 *
 * \file : NUR_strategy.c
 *
 * \brief : Strat�gie sur le bloc qui n�a pas (de pr�f�rence, pas du tout) �t� utilis� r�cemment.
 * 
 * \author : Yasin EROGLU - Jonathan BOUDAB - Ugo EL KOUBI - Th�o DONZELLE
 *
 */

#include "strategy.h"
#include "low_cache.h"

 // Flag pour le bit de r�f�rence
#define REFER 0x4

// NDEREF est la p�riode avec laquelle le bit R est remis � 0
#define NDEREF 100


 /* Cr�ation d'une structure avec cptderf : le compteur de d�f�ren�age et 
 nderef : p�riode entre deux d�r�ren�ages*/
struct NUR_Strategy
{
    unsigned nderef;    /* P�riode de d�r�f�ren�age */
    unsigned cptderef;  /* compteur */
};

 // D�r�f�rencage
static void Dereferencage(struct Cache *pcache)
{
    struct NUR_Strategy *pstrat = (struct NUR_Strategy *)(pcache)->pstrategy;
    int ib;

    // D�f�rencement tous les deref acc�s
    if (--pstrat->cptderef > 0){
        return;
    }
    // Remise � zero de tous les bits de d�f�rence
    for (ib = 0; ib < pcache->nblocks; ib++){
        pcache->headers[ib].flags &= ~REFER;
    }
    // Remise a 0 du compteur
    pstrat->cptderef = pstrat->nderef;
    // Incr�mentation du compteur de d�r�f�ren�age
    ++pcache->instrument.n_deref;
}



 // Initialisation de la strat�gie.
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

// Fermeture de la strat�gie.
void Strategy_Close(struct Cache *pcache)
{
    free(pcache->pstrategy);
}

// Invalidation du cache
void Strategy_Invalidate(struct Cache *pcache) 
{
    struct NUR_Strategy *pstrat = (struct NUR_Strategy *)(pcache)->pstrategy;
    // Si nderef est nul on ne d�f�rence jamais
    if (pstrat->nderef != 0) {
        pstrat->cptderef = 1;
        Dereferencage(pcache);
    }   
}

 /* On cherche un bloc invilade, si il n'y en a pas pas on prend le bloc
 donc le nombre RM calcul� est le plus petit
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
        // Construction du nombre RM avec le bit de r�f�rence et le bit de modification (RM = 2*R+M)
        rm = calculateRM(pbh->flags & REFER, pbh->flags & MODIF);
        if (rm < min) {
            min = rm;
            best_pbh = pbh;
        }   
    }
    return best_pbh;    
}


 // Si une lecture ou une ecriture est faite, on d�r�f�rence si il y a besoin et on met le bit R � 1
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

//! Identification de la strat�gie.
char *Strategy_Name()
{
    return "NUR";
}



