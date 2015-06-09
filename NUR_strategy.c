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

/* Constantes
 * ----------
*/

/* Le flag indicant une référence. On utilisera directement le champ flags de la
 * Cache_Block_Header (voir low_cache.h)
*/
#define REFER 0x4

/* Période de mise à zéro des bits de référence */
#define NDEREF 100

/* Macros utiles
 * ------------- 
*/

/* Valeur de vérité d'un entier */
#define BOOLVAL(v) ((v) ? 1 : 0)

/* Prend 2 bits m et r et en fait un entier m*2 + r */  
#define MAKE_RM(r, m) ((BOOLVAL(r)<<1) | BOOLVAL(m))

/* Structure de données propre à la stratégie NUR */
struct NUR_Strategy
{
    unsigned nderef;    /* Période de déréférençage */
    unsigned cptderef;  /* compteur */
};
#define NUR_STRATEGY(pcache) ((struct NUR_Strategy *)(pcache)->pstrategy)

/* Fonction définie plus loin */
static void Dereference_If_Needed(struct Cache *pcache);

/* ------------------------------------------------------------------
 * Fonctions externes (utlisées par cache.c)
 * ------------------------------------------------------------------
 */

/* initialisation de la stratégie
 * ------------------------------
 * On alloue une structure contenant le compteur de déréférençage (cptderf)
 * ainsi que la valeur maximale de ce dernier (nderef) qui constitue la
 * "période" de déréférençage.
*/
void *Strategy_Create(struct Cache *pcache) 
{
    struct NUR_Strategy *pstrat = malloc(sizeof(struct NUR_Strategy));

    pstrat->nderef = pcache->nderef;
    pstrat->cptderef = pcache->nderef;

    return pstrat;
}

/* Fermeture de la stratégie
 * -------------------------
 */
void Strategy_Close(struct Cache *pcache)
{
    free(pcache->pstrategy);
}

/* Invalidation du cache
 * ---------------------
 * On force la remise à zéro des bits de référence.
*/
void Strategy_Invalidate(struct Cache *pcache) 
{
    struct NUR_Strategy *pstrat = NUR_STRATEGY(pcache);

    if (pstrat->nderef != 0) 
    {
        pstrat->cptderef = 1;
        Dereference_If_Needed(pcache);
    }   
}

/* Algorithme de remplacement de bloc
 * ---------------------------------- 
 * On prend le premier bloc invalide. S'il n'y en a plus on cherche un bloc non
 * référencé dans la dernière tranche de temps (et de préférence non modifié).
 */
struct Cache_Block_Header *Strategy_Replace_Block(struct Cache *pcache) 
{
    int ib;
    int min;
    struct Cache_Block_Header *pbh_save = NULL;
    struct  Cache_Block_Header *pbh;
    int rm;

   /* On cherche d'abord un bloc invalide */
    if ((pbh = Get_Free_Block(pcache)) != NULL) return pbh;

    for (min = MAKE_RM(1, 1) + 1, ib = 0; ib < pcache->nblocks; ib++)
    {
        pbh = &pcache->headers[ib];
        
        /* On construit un nombre binaire rm avec le bit de modification
             * et le bit de référence, et on cherche le bloc avec la valeur de rm
             * minimale */
        rm = MAKE_RM(pbh->flags & REFER, pbh->flags & MODIF);

        if (rm == 0) {
            return pbh; /* pas la peine de cherche plus loin */
        } else if (rm < min) {
            min = rm;
            pbh_save = pbh;
        }   
    }
    
    return pbh_save;    
}

/* Fonctions "réflexes" en cas de lecture et d'écriture
 * ---------------------------------------------------- 
 * On se contente ici de déréférencer si besoin et surtout de marquer le bloc
 * comme ayant été référencé
 */
void Strategy_Read(struct Cache *pcache, struct Cache_Block_Header *pbh) 
{
    Dereference_If_Needed(pcache);
    pbh->flags |= REFER;
} 
  
void Strategy_Write(struct Cache *pcache, struct Cache_Block_Header *pbh)
{
    Dereference_If_Needed(pcache);
    pbh->flags |= REFER;
} 

/* Identification de la stratégie
 * ------------------------------
 */
char *Strategy_Name()
{
    return "NUR";
}

/* ------------------------------------------------------------------
 * Fonctions purement locales (privées) à ce module
 * ------------------------------------------------------------------
 */

/* Déréférençage si besoin
 * -----------------------
 * On remet à 0 le bit de référence tous les deref accès.
 */
static void Dereference_If_Needed(struct Cache *pcache)
{
    struct NUR_Strategy *pstrat = NUR_STRATEGY(pcache);
    int ib;

    /* On déréférence tous les deref accès ; si deref est 0 on ne déréférence jamais */
    if (pstrat->nderef == 0 || --pstrat->cptderef > 0) return;

    /* C'est le moment : on remet à 0 tous les bits de déférence */
    for (ib = 0; ib < pcache->nblocks; ib++) pcache->headers[ib].flags &= ~REFER;

    /* On réarme le compteur et on met à jour l'instrumentation */
    pstrat->cptderef = pstrat->nderef;
    ++pcache->instrument.n_deref;
}

