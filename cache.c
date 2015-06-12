/*!
 *
 * \file : cache.c
 *
 * \brief : Gestion générale du cache.
 * 
 * \author : Yasin EROGLU - Jonathan BOUDAB - Ugo EL KOUBI - Théo DONZELLE
 *
 */

#include "cache.h"
#include "low_cache.h"
#include "strategy.h"
#include <string.h>
#include <stdio.h>
#include <unistd.h>
#include <assert.h>
 
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

    // Allocation du cache
    struct Cache *pcache = (struct Cache *) malloc (sizeof(struct Cache));
    pcache->file = malloc(strlen(fic) + 1);
    strcpy(pcache->file, fic);
    pcache->fp = fopen(fic, "w+");

    // Initialisation dimension
    pcache->nblocks = nblocks;
    pcache->nrecords = nrecords;
    pcache->recordsz = recordsz;
    pcache->nderef = nderef;
    pcache->blocksz = nrecords*recordsz;

    // Initialisation stratégie
    pcache->pstrategy = Strategy_Create(pcache);

    // Allocation des headers
    pcache->headers = malloc(sizeof(struct Cache_Block_Header) * nblocks);
    
    // Initialisation des headers
    for(int i = 0; i < nblocks; i++) {
        pcache->headers[i].flags = 0x0;
        pcache->headers[i].ibfile = 0;
        pcache->headers[i].ibcache = i;
        pcache->headers[i].data = malloc(pcache->blocksz);
    }

    // Initialisation du premier bloc libre
    pcache->pfree = pcache->headers;

    // Initialisation de l'instrumentation du cache
    pcache->instrument.n_reads = 0;
    pcache->instrument.n_writes = 0;
    pcache->instrument.n_hits = 0;
    pcache->instrument.n_syncs = 0;
    pcache->instrument.n_deref = 0;

    cptNSYNC = 0;

    return pcache;
}

/* 
 * Détruit le cache pointé par pcache : synchronise le cache et le fichier grâce à
 * Cache_Sync(), ferme le fichier et détruit toutes les structures de données du 
 * cache.
 */
Cache_Error Cache_Close(struct Cache *pcache) {

    // Synchronise
    if(Cache_Sync(pcache) == CACHE_KO) {
        return CACHE_KO;
    }

    // Choix de la stratégie
    Strategy_Close(pcache);

    if(fclose(pcache->fp) != 0) {
        return CACHE_KO;
    }

    // Deallocation des differents blocks
    for(int i = 0; i < pcache->nblocks; i++) {
        free(pcache->headers[i].data);
    }

    free(pcache->headers);
    free(pcache->file);
    free(pcache);

    return CACHE_OK;
}

/* 
 * Synchronise le contenu du cache avec celui du fichier : écrit sur disque tous les
 * blocs dont le bit M vaut 1 et remet à 0 ce bit. L’application peut appeler 
 * Cache_Sync() quand elle le souhaite, mais il y a un appel automatique tous les
 * NSYNC accès (par défaut NSYNC vaut 1000, défini dans low_cache.c).
 */
Cache_Error Cache_Sync(struct Cache *pcache) {

    // On parcour tous les blocks.
    for(int i = 0; i < pcache->nblocks; i++) {
        
        // Si le bloc à dans son flag le bit de modif à 1 alors on copie le bloc dans le fichier.
        if((pcache->headers[i].flags & (MODIF|VALID)) != 0) {
            
            // On se décale dans le fichier à l'endroit où effectuer la modification.
            // Si le dacalage n'a pas marché, on retourne le code d'erreur.
            if(fseek(pcache->fp, DADDR(pcache, pcache->headers[i].ibfile), SEEK_SET) != 0) {
                return CACHE_KO;
            }
            
            // On remplace donc les données du fichier par celle du bloc.
            // Si le remplacement n'a pas marché, on retourne le code d'erreur.
            if(fwrite(pcache->headers[i].data, 1, pcache->blocksz, pcache->fp) != pcache->blocksz) {
                return CACHE_KO;
            }
            
            // Si tout à marché, on remet donc le bit de modifaction à 0
            pcache->headers[i].flags &= ~MODIF;
        }
    }

    // Incrémentation du compteur de synchronisation
    pcache->instrument.n_syncs++;
    cptNSYNC = 0;
    
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
    
    // Synchronise
    if(Cache_Sync(pcache) == CACHE_KO) {
        return CACHE_KO;
    }

    // Met en invalides tous les blocs
    for(int i = 0; i < pcache->nblocks; i++){
        pcache->headers[i].flags &= ~VALID;
    }
    
    // Initialisation du premier bloc libre
    pcache->pfree = pcache->headers;

    // Choix de la stratégie
    Strategy_Invalidate(pcache); 
    
    return CACHE_OK;
}

/* 
 * Retourne le bloc valide du cache sinon retourne Null
 */
struct Cache_Block_Header * BlockValidInCache(struct Cache *pcache, int irfile) {
    
    // Indice de l'enregistrement
    int ibSearch = irfile / pcache->nrecords;

    // Parcours tous les blocks
    for(int i = 0 ; i < pcache->nblocks ; ++i) {
        
        // Vérifie si le block est valide
        if(pcache->headers[i].flags & VALID) {
            
            // Vérifie s'il contient l'enregistrement
            if(pcache->headers[i].ibfile == ibSearch) {
                pcache->instrument.n_hits++;
                return &(pcache->headers[i]);
            }
        }
    }

    return NULL;
}

Cache_Error WriteInBlock(struct Cache *pcache, struct Cache_Block_Header *header) {
    
    // Positionne le pointeur à l'adresse du block
    if(fseek(pcache->fp, DADDR(pcache, header->ibfile), SEEK_SET) != 0) {
        return CACHE_KO;
    }

    // Ecriture dans le block
    if(fwrite(header->data, 1, pcache->blocksz, pcache->fp) != pcache->blocksz) {
        return CACHE_KO;
    }

    // Enlève le flag modif
    header->flags &= ~MODIF;
    
    return CACHE_OK;
}

Cache_Error ReadInBlock(struct Cache *pcache, struct Cache_Block_Header *header) {

    // Recherche la longueur du fichier
    if(fseek(pcache->fp, 0, SEEK_END) < 0) {
        return CACHE_KO;
    }

    // Crée un block à 0 si on est sortie du fichier
    if(DADDR(pcache, header->ibfile) >= ftell(pcache->fp)) {
        memset(header->data, '\0', pcache->blocksz);
    } else {
        
        // Positionne le pointeur à l'adresse du block
        if(fseek(pcache->fp, DADDR(pcache, header->ibfile), SEEK_SET) != 0) {
            return CACHE_KO;
        }
        
        // Lecture dans le block
        if(fread(header->data, 1, pcache->blocksz, pcache->fp) != pcache->blocksz) {
            return CACHE_KO;
        }
    }
    
    // Met le flag valid
    header->flags |= VALID;
    
    return CACHE_OK;
}

/* 
 * Lecture à travers le cache de l’enregistrement d’indice irfile dans le fichier.
 * Le paramètre precord doit pointer sur un buffer fourni par l’application et au 
 * moins de taille recordsz. L’enregistrement sera transféré du cache dans ce buffer
 * pour une lecture. 
 */
Cache_Error Cache_Read(struct Cache *pcache, int irfile, void *precord) {

    // +1 au nombre de lecture
    pcache->instrument.n_reads++;

    // Stock le pointeur sur l'entête du block qui contient l'enregistrement
    struct Cache_Block_Header *header = BlockValidInCache(pcache, irfile);
    
    // Si le block n'est pas dans le cache
    if(header == NULL) {
        // Bloc est récupéré par la stratégies
        header = Strategy_Replace_Block(pcache);
        
        if((header->flags & VALID) && (header->flags & MODIF) && WriteInBlock(pcache, header) != CACHE_OK) {
            return CACHE_KO;
        }

        header->flags = 0;
        header->ibfile = irfile / pcache->nrecords;
        
        if(ReadInBlock(pcache, header) != CACHE_OK) {
            return CACHE_KO;
        }
        
        if(header == NULL) {
            return CACHE_KO;
        }
    }

    // On copie dans le buffer
    memcpy(precord, ADDR(pcache, irfile, header) , pcache->recordsz);

    // Synchronise
    if(++cptNSYNC == NSYNC) {
        Cache_Sync(pcache);
    }
    
    // Choix stratégie
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

    // +1 au nombre d'écriture
    pcache->instrument.n_writes++;

    // Stock le pointeur sur l'entête du block qui contient l'enregistrement
    struct Cache_Block_Header * header = BlockValidInCache(pcache, irfile);
    
    // Si le block n'est pas dans le cache
    if(header == NULL) {
        // Bloc est récupéré par la stratégies
        header = Strategy_Replace_Block(pcache);
        
        if((header->flags & VALID) && (header->flags & MODIF) && WriteInBlock(pcache, header) != CACHE_OK) {
            return CACHE_KO;
        }

        header->flags = 0;
        header->ibfile = irfile / pcache->nrecords;
        
        if(ReadInBlock(pcache, header) != CACHE_OK) {
            return CACHE_KO;
        }

        if(header == NULL) {
            return CACHE_KO;
        }
    }

    // On copie dans le buffer
    memcpy(ADDR(pcache, irfile, header), precord, pcache->recordsz);

    header->flags |= MODIF;

    // Synchronise
    if(++cptNSYNC == NSYNC) {
        Cache_Sync(pcache);
    }

    // Choix stratégie
    Strategy_Write(pcache, header);

    return CACHE_OK;
}

/* 
 * Récupère un pointeur sur une copie des statistiques courantes.
 * Il retourne une copie de la structure d'instruction du cache pointé par
 * pcache.
 */
struct Cache_Instrument *Cache_Get_Instrument(struct Cache *pcache) {

    static struct Cache_Instrument instrumentCopy;
    instrumentCopy = pcache->instrument;

    pcache->instrument.n_reads = 0;
    pcache->instrument.n_writes = 0;
    pcache->instrument.n_hits = 0;
    pcache->instrument.n_syncs = 0;
    pcache->instrument.n_deref = 0;
    
    return &instrumentCopy; 
}
