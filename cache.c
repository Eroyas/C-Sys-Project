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
	struct Cache *pcache = malloc (sizeof(struct Cache *));
	pcache->file = fic;
	pcache->fp = fopen(fic, "r+"); // surement changer "r+" par "w+"
	pcache->nblocks = nblocks;
	pcache->nrecords = nrecords;
	pcache->recordsz = recordsz;
	pcache->blocksz = nrecords*recordsz;
	pcache->instrument->n_reads = 0;
	pcache->instrument->n_writes = 0;
	pcache->instrument->n_hits = 0;
	pcache->instrument->n_syncs = 0;
	pcache->instrument->n_deref = 0;
	pcache->pfree->malloc(sizeof(struct Cache_Block_Header *));
	pcache->headers->malloc(sizeof(struct Cache_Block_Header *) * nblocks);
	// Dans headers : différence entre ibfile et ibcache ?
	// Struct Cache_flags
}

/* 
 * Détruit le cache pointé par pcache : synchronise le cache et le fichier grâce à
 * Cache_Sync(), ferme le fichier et détruit toutes les structures de données du 
 * cache.
 */
int Cache_Close(struct Cache *pcache) {
	Cache_Sync(*pcache);
	fclose(pcache->fp);
	free(pcache);
}

/* 
 * Synchronise le contenu du cache avec celui du fichier : écrit sur disque tous les
 * blocs dont le bit M vaut 1 et remet à 0 ce bit. L’application peut appeler 
 * Cache_Sync() quand elle le souhaite, mais il y a un appel automatique tous les
 * NSYNC accès (par défaut NSYNC vaut 1000, défini dans low_cache.c).
 */
Cache_Error Cache_Sync(struct Cache *pcache) {

}

/* 
 * Invalide le cache, c’est-à-dire met à 0 le bit V de tous les blocs. C’est donc 
 * comme si le cache était vide : aucun bloc ne contient plus d’information utile.
 * Noter que cette fonction ne devrait pas faire partie de l’interface utilisateur 
 * du cache. Néanmoins, elle est nécessaire au simulateur, puisqu’elle permet
 * d’enchainer des tests différents sans avoir à réallouer le cache.
 */
Cache_Error Cache_Invalidate(struct Cache *pcache) {

}

/* 
 * Lecture à travers le cache de l’enregistrement d’indice irfile dans le fichier.
 * Le paramètre precord doit pointer sur un buffer fourni par l’application et au 
 * moins de taille recordsz. L’enregistrement sera transféré du cache dans ce buffer
 * pour une lecture. 
 */
Cache_Error Cache_Read(struct Cache *pcache, int irfile, void *precord) {

}

/* 
 * L'écriture à travers le cache de l’enregistrement d’indice irfile dans le fichier.
 * Le paramètre precord doit pointer sur un buffer fourni par l’application et au 
 * moins de taille recordsz. L’enregistrement sera transféré du buffer vers le cache
 * pour une écriture.
 */
Cache_Error Cache_Write(struct Cache *pcache, int irfile, const void *precord) {

}

/* 
 * Récupère un pointeur sur une copie des statistiques courantes.
 * Il retourne une copie de la structure d'instruction du cache pointé par
 * pcache.
 */
struct Cache_Instrument *Cache_Get_Instrument(struct Cache *pcache) {
	struct Cache_Instrument instrumentCopy = malloc(sizeof(struct Cache_Instrument));
	instrumentCopy = pcache->instrument;
	return (instrumentCopy);
}
