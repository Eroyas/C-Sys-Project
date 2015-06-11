/*!
 *
 * \file : low_cache.c
 *
 * \brief : Structures de données pour l'implémentation du cache.
 * 
 * \author : Yasin EROGLU - Jonathan BOUDAB - Ugo EL KOUBI - Théo DONZELLE
 *
 */

#include "cache.h"
#include "low_cache.h"
#include "strategy.h"
#include <string.h>

 int cptNSYNC;

/* 
 * Crée un cache associé au fichier de nom fic : la cache comporte nblocks, chaque
 * bloc contenant nrecords enregistrements de taille recordsz caractères.
 * Le dernier paramètre (nderef) n’est utilisé que pour la stratégie nur. Pour les 
 * autres stratégies sa valeur est ignorée. Dans le cas de nur, le bit de référence
 * R devra être remis à 0 (pour tous les blocs du cache) tous les nderef accès
 * (lecture ou écriture).
 * La fonction ouvre le fichier en le créant et le remplissant si nécessaire, alloue
 * et initialise les structures de données du cache, et retourne un pointeur sur le 
 * nouveau cache.
 */
struct Cache *Cache_Create(const char *fic, unsigned nblocks, unsigned nrecords,
                           size_t recordsz, unsigned nderef) {

    struct Cache *pcache = malloc (sizeof(struct Cache));
    pcache->file = strdup(fic);
    pcache->fp = fopen(fic, "r+"); // surement changer "r+" par "w+"
    pcache->nblocks = nblocks;
    pcache->nrecords = nrecords;
    pcache->recordsz = recordsz;
    pcache->blocksz = nrecords*recordsz;
    pcache->instrument.n_reads = 0;
    pcache->instrument.n_writes = 0;
    pcache->instrument.n_hits = 0;
    pcache->instrument.n_syncs = 0;
    pcache->instrument.n_deref = 0;
    pcache->pstrategy = NULL;
    pcache->headers = malloc(sizeof(struct Cache_Block_Header) * nblocks);
    for(int i = 0; i < nblocks; i++){
        pcache->headers[i].flags = 0x0;
        pcache->headers[i].ibfile = -1;
        pcache->headers[i].ibcache = i;
        pcache->headers[i].data = malloc(pcache->blocksz);
    }
    pcache->pfree = Get_Free_Block(pcache);
    return pcache;
}

/* 
 * Détruit le cache pointé par pcache : synchronise le cache et le fichier grâce à
 * Cache_Sync(), ferme le fichier et détruit toutes les structures de données du 
 * cache.
 */
Cache_Error Cache_Close(struct Cache *pcache) {

    if (Cache_Sync(pcache) == CACHE_KO) return CACHE_KO;
    if  (fclose(pcache->fp) != 0) return CACHE_KO;
    for (int i = 0; i<pcache->nblocks; i++)
    	free(pcache->headers[i].data);
    free(pcache->headers);
    free(pcache);
    printf("salut 2");
    return CACHE_OK;
}

/* 
 * Synchronise le contenu du cache avec celui du fichier : écrit sur disque tous les
 * blocs dont le bit M vaut 1 et remet à 0 ce bit. L’application peut appeler 
 * Cache_Sync() quand elle le souhaite, mais il y a un appel automatique tous les
 * NSYNC accès (par défaut NSYNC vaut 1000, défini dans low_cache.c).
 */
Cache_Error Cache_Sync(struct Cache *pcache) {
    // Incrémentation du compteur de synchronisation
    pcache->instrument.n_syncs++;

    // On parcour tous les blocks.
    for(int i = 0; i < pcache->nblocks; i++){
        // Si le bloc à dans son flag le bit de modif à 1 alors on copie le bloc dans le fichier.
        if((pcache->headers[i].flags & MODIF) != 0){
            // On se décale dans le fichier à l'endroit où effectuer la modification.
            // Si le dacalage n'a pas marché, on retourne le code d'erreur.
            if(fseek(pcache->fp, pcache->headers[i].ibfile * pcache->blocksz, SEEK_SET) != 0){
                return CACHE_KO;
            }
            // On remplace donc les données du fichier par celle du bloc.
            // Si le remplacement n'a pas marché, on retourne le code d'erreur.
            if(fputs(pcache->headers[i].data, pcache->fp) == EOF){
                return CACHE_KO;
            }
            // Si tout à marché, on remet donc le bit de modifaction à 0
            pcache->headers[i].flags &= ~MODIF;
        }
    }
    // On retourne ensuite le code de réussite.
    return CACHE_OK;
}

/* 
 * Invalide le cache, c’est-à-dire met à 0 le bit V de tous les blocs. C’est donc 
 * comme si le cache était vide : aucun bloc ne contient plus d’information utile.
 * Noter que cette fonction ne devrait pas faire partie de l’interface utilisateur 
 * du cache. Néanmoins, elle est nécessaire au simulateur, puisqu’elle permet
 * d’enchainer  des tests différents sans avoir à réallouer le cache.
 */
Cache_Error Cache_Invalidate(struct Cache *pcache) {

    int max = pcache->nblocks;
    for(int i = 0; i < max; i++){
        pcache->headers[i].flags &= ~VALID;
    }
    return CACHE_OK;
}

/* 
 * Retourne le bloc valide du cache sinon retourne Null
 */
struct Cache_Block_Header * BlockValidInCache(struct Cache *pcache, int irfile) {
    
    int ibSearch = irfile / pcache->nrecords;

    for(int i = 0 ; i < pcache->nblocks ; ++i) {
        if(pcache->headers[i].flags & VALID) {
            if(pcache->headers[i].ibfile == ibSearch)
                return &(pcache->headers[i]);
        }
    }

    return NULL;
}

/* 
 * Lecture à travers le cache de l’enregistrement d’indice irfile dans le fichier.
 * Le paramètre precord doit pointer sur un buffer fourni par l’application et au 
 * moins de taille recordsz. L’enregistrement sera transféré du cache dans ce buffer
 * pour une lecture. 
 */
Cache_Error Cache_Read(struct Cache *pcache, int irfile, void *precord) {

    struct Cache_Block_Header *header = BlockValidInCache(pcache, irfile);
    
    // Si le block n'est pas dans le cache
    if(header == NULL) {
        // Bloc est récupéré par la stratégies
        header = Strategy_Replace_Block(pcache);
        header->ibfile = irfile / pcache->nrecords;

        // Permet de placer le curseur à une position donnée pour réaliser un accès 
        // direct dans un fichier. La nouvelle position sera decalée d'un nombre d'octets 
        // égal au paramètre deplacement (deuxieme param) depuis le début du fichier (si
        // reference, le derrnier param, vaut SEEK_SET (valant en fait 0))
        // Le premier param est le pointeur vers le fichier en question.
        if(fseek(pcache->fp, DADDR(pcache, header->ibfile), SEEK_SET) != 0) {
            return CACHE_KO;
        }

        if(fgets(header->data, pcache->blocksz, pcache->fp) == EOF) {
            return CACHE_KO;
        }

        // MAJ des flags
        header->flags |= VALID; // n'est plus free
        header->flags &= ~MODIF; // mets le flag de modification
    } else {
        // l'élément est dans le cache
        pcache->instrument.n_hits++;
    }
    // on copie dans le buffer
    memcpy(precord, ADDR(pcache, irfile, header) , pcache->recordsz);
    // +1 au nombre de lecture
    pcache->instrument.n_reads++;

    if(++cptNSYNC == NSYNC) {
        Cache_Sync(pcache);
    }
    
    Strategy_Read(pcache, header);

    return CACHE_OK;
}

/* 
 * L'écriture à travers le cache de l’enregistrement d’indice irfile dans le fichier.
 * Le paramètre precord doit pointer sur un buffer fourni par l’application et au 
 * moins de taille recordsz. L’enregistrement sera transféré du buffer vers le cache
 * pour une écriture.
 */
Cache_Error Cache_Write(struct Cache *pcache, int irfile, const void *precord) {

    struct Cache_Block_Header * header = BlockValidInCache(pcache, irfile);
    
    // si le block n'est pas dans le cache
    if(header == NULL) {
        header = Strategy_Replace_Block(pcache);
        pcache->pfree = Get_Free_Block(pcache);
        header->ibfile = irfile / pcache->nrecords;

        if(fseek(pcache->fp, DADDR(pcache, header->ibfile), SEEK_SET) != 0) {
            return CACHE_KO;
        }

        if(fgets(header->data, pcache->blocksz, pcache->fp) == EOF) {
            return CACHE_KO;
        }

        // MAJ des flags
        header->flags |= VALID; // n'est plus free
        header->flags &= ~MODIF; // mets le flag de modification
    } else {
        // l'élement est dans le cache
        pcache->instrument.n_hits++;
    }

    // on copie dans le bloc
    // if(fgets((char *)ADDR(pcache, irfile, header), pcache->recordsz, precord)== EOF) return CACHE_KO;
    memcpy(ADDR(pcache, irfile, header), precord, pcache->recordsz);

    header->flags |= MODIF;
    // +1 au nombre d'écriture
    pcache->instrument.n_writes++;
    Cache_Sync(pcache);

    if(++cptNSYNC == NSYNC) {
        Cache_Sync(pcache);
    }

    Strategy_Write(pcache, header);

    return CACHE_OK;
}

/* 
 * Récupère un pointeur sur une copie des statistiques courantes.
 * Il retourne une copie de la structure d'instruction du cache pointé par
 * pcache.
 *Attention : tous les compteurs de la structure courante sont remis à 0 par cette
 *fonction.
 */
struct Cache_Instrument *Cache_Get_Instrument(struct Cache *pcache) {

    struct Cache_Instrument *instrumentCopy = malloc(sizeof(struct Cache_Instrument));
    *instrumentCopy = pcache->instrument;
    pcache->instrument.n_reads = 0;
    pcache->instrument.n_writes = 0;
    pcache->instrument.n_hits = 0;
    pcache->instrument.n_syncs = 0;
    pcache->instrument.n_deref = 0;
    return (instrumentCopy);
}
