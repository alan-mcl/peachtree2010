/* MapKit void* -> int */

/*
 * Copyright (c) 2002-2005, Jean-Sebastien Roy (js@jeannot.org)
 * 
 * Permission is hereby granted, free of charge, to any person obtaining a
 * copy of this software and associated documentation files (the
 * "Software"), to deal in the Software without restriction, including
 * without limitation the rights to use, copy, modify, merge, publish,
 * distribute, sublicense, and/or sell copies of the Software, and to
 * permit persons to whom the Software is furnished to do so, subject to
 * the following conditions:
 * 
 * The above copyright notice and this permission notice shall be included
 * in all copies or substantial portions of the Software.
 * 
 * THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS
 * OR IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF
 * MERCHANTABILITY, FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT.
 * IN NO EVENT SHALL THE AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY
 * CLAIM, DAMAGES OR OTHER LIABILITY, WHETHER IN AN ACTION OF CONTRACT,
 * TORT OR OTHERWISE, ARISING FROM, OUT OF OR IN CONNECTION WITH THE
 * SOFTWARE OR THE USE OR OTHER DEALINGS IN THE SOFTWARE.
 */

/* @(#) $Jeannot: mapkit_vpi.h,v 1.4 2005/05/05 09:23:51 js Exp $ */
/*
  MapKit
  Copyright (c) 2002-2005, Jean-Sebastien Roy (js@jeannot.org)
  @(#) $Jeannot: mapkit_vpi.h,v 1.4 2005/05/05 09:23:51 js Exp $
*/

/* @(#) $Jeannot: mapkit_vpi.h,v 1.4 2005/05/05 09:23:51 js Exp $ */


#ifndef _MAPKIT_VPI_
#define _MAPKIT_VPI_

#include "mapkit_generic.h"

/* MS Visual C does not recognize 'inline' */
#if defined(_MSC_VER) && ! defined(__cplusplus) && ! defined(inline)
#define inline __inline
#endif

#if defined(__cplusplus)
extern "C" {
#endif

/* Map structures */

/*
  Prototypes for map_vpi (void* -> int)
  Default value : 0
  Uses a state field.
*/

typedef struct
{
  signed char state;
  void* key;
  int value;
}
map_vpi_storage;

typedef struct
{
  void* key;
  int value;
}
map_vpi_element;

typedef struct
{
  mapkit_size_t size, /* size of the hash table */
    fill, /* # of non-free slots in the hash table */
    used, /* # of full slots */
    maxfill, /* max # of non-free slots before rehash */
    minused /* min # of full slots before rehash */;

  double maxfillfactor, /* maxfill = maxfillfactor * size */
    minusedfactor; /* minused = minusedfactor * size */

#ifdef MAPKIT_COLLISIONS
  unsigned long insertionindexs, insertionindex_collisions;
  unsigned long keyindexs, keyindex_collisions;
#endif

  int defaultvalue;

  int alwaysdefault;

  map_vpi_storage *contents;
}
map_vpi;

/* Macros */

/* set to inform the type is available */
#define MAPKIT_map_vpi

/* read/write access to the value at key (insert as needed) */
#define map_vpi_(spm, key) (*(map_vpi_insertptr(spm, key)))

/* true if key exists */
#define map_vpi_haskey(spm, key) ((map_vpi_ptr(spm, key)) != NULL)

/* Prototypes */

/* initialize the structure */
extern mapkit_error map_vpi_init(map_vpi *spm);

/* free contents */
extern void map_vpi_free(map_vpi *spm);

/* copy the "from" map into a new "to" map */
extern mapkit_error map_vpi_copy(map_vpi *to, map_vpi *from);

/* initialize the structure for "used" elements */
extern mapkit_error map_vpi_init_hint(map_vpi *spm, mapkit_size_t used);

/* ensure at least "used" slots (including currently occupied slots) */
extern mapkit_error map_vpi_ensurecapacity(map_vpi *spm, mapkit_size_t used);

/* restore maxfill & minused and ensure these constraints */
extern mapkit_error map_vpi_adjustcapacity(map_vpi *spm);

/* reallocate contents for "size" slots, keeping previous values */
extern mapkit_error map_vpi_reallocate(map_vpi *spm, mapkit_size_t size);

/* return an adequate size for growing from "used" elements */
extern mapkit_size_t map_vpi_growsize(map_vpi *spm, mapkit_size_t used);

/* return an adequate size for shrinking from "used" elements */
extern mapkit_size_t map_vpi_shrinksize(map_vpi *spm, mapkit_size_t used);

/* return an adequate size for "used" elements */
extern mapkit_size_t map_vpi_meansize(map_vpi *spm, mapkit_size_t used);

/* pointer to the value at key (insert as needed) */
static inline int *map_vpi_insertptr(map_vpi *spm, void* key);

/* pointer to the value at key or NULL */
static inline int *map_vpi_ptr(map_vpi *spm, void* key);

/* remove the key pointing to the value *ptr. No checking is done */
static inline mapkit_error map_vpi_removeptr(map_vpi *spm, int *ptr);

/* set the value at key (insert as needed) */
static inline mapkit_error map_vpi_set(map_vpi *spm,
  void* key, int value);

/* return the value at key (key must exists if alwaysdefaultvalue is not set) */
static inline int map_vpi_value(map_vpi *spm,
  void* key);

/* get the value at key (return an error code) */
static inline mapkit_error map_vpi_get(map_vpi *spm, void* key,
  int *value);

/* remove key (key must exists) */
static inline mapkit_error map_vpi_remove(map_vpi *spm,
  void* key);

/*
 * return the next index to a full slot of the map, or -1 if none
 * if index == -1, returns the first index, or -1 if none
 * the contents of the slot can be accessed by spm->contents[index].key and
 * spm->contents[index].value
 */
extern mapkit_size_t map_vpi_next(map_vpi *spm, mapkit_size_t index);

/*
 * return the next ptr to a full slot of the map, or NULL if none
 * if pos_contents == NULL, returns the first full slot, or NULL if none
 * the contents of the slot can be accessed by pos_contents->key and
 * pos_contents->value
 */
extern map_vpi_storage *map_vpi_nextptr(map_vpi *spm, map_vpi_storage *pos_contents);

/* allocate an array of map_vpi_element with all "count" (key,value) pairs */
extern mapkit_error map_vpi_getall(map_vpi *spm, map_vpi_element **array,
  mapkit_size_t *count);

/* remove all values equal to the defaultvalue */
extern mapkit_error map_vpi_clean(map_vpi *spm);

/* compare the key of two map_vpi_element (for qsort) */
extern int map_vpi_compare(const void *e1, const void *e2);

/* allocate an array of map_vpi_element with all "count" (key,value) pairs */
extern mapkit_error map_vpi_getall_sorted(map_vpi *spm,
  map_vpi_element **array, mapkit_size_t *count);

/* insert all count map_vpi_element into spm */
extern mapkit_error map_vpi_setall(map_vpi *spm, map_vpi_element *array,
  mapkit_size_t count);

/* remove all "count" elements whose keys are in "array" */
extern mapkit_error map_vpi_removeall(map_vpi *spm, void* *array,
  mapkit_size_t count);

/* print statistics */
extern void map_vpi_printstats(map_vpi *spm);

/* Private functions */

extern mapkit_error map_vpi_remove_s(map_vpi *spm, void* key);
extern int map_vpi_value_s(map_vpi *spm, void* key);
extern mapkit_error map_vpi_get_s(map_vpi *spm, void* key, int *value);
extern mapkit_error map_vpi_set_s(map_vpi *spm, void* key, int value);
extern int *map_vpi_insertptr_s(map_vpi *spm, void* key);
extern int *map_vpi_ptr_s(map_vpi *spm, void* key);

/* Implementation */

/* static inlined functions */
mapkit_error map_vpi_remove(map_vpi *spm, void* key)
{
  map_vpi_storage *contents;
    
  contents = &(spm->contents[((mapkit_hash_t)key) % spm->size]);

  if ((contents->state == MAPKIT_FULLSLOT)
    && ((contents->key) == (key)))
  {
    contents->state = MAPKIT_DELETEDSLOT;
#ifdef MAPKIT_COLLISIONS
    spm->keyindexs ++;
#endif
    spm->used --;
    if (spm->used < spm->minused)
    {
#ifdef MAPKIT_DEBUG
      fprintf(stderr, "MAPKIT: used < minused\n");
#endif
      return map_vpi_reallocate(spm, map_vpi_shrinksize(spm, spm->used));
    }
  }
  else
    return map_vpi_remove_s(spm, key);
  return MAPKIT_OK;
}

mapkit_error map_vpi_removeptr(map_vpi *spm, int *ptr)
{
  map_vpi_storage *sptr = (map_vpi_storage *)((char *)ptr - offsetof(map_vpi_storage,value));
  sptr->state = MAPKIT_DELETEDSLOT;
  spm->used --;
  if (spm->used < spm->minused)
  {
#ifdef MAPKIT_DEBUG
    fprintf(stderr, "MAPKIT: used < minused\n");
#endif
    return map_vpi_reallocate(spm, map_vpi_shrinksize(spm, spm->used));
  }

  return MAPKIT_OK;
}

mapkit_error map_vpi_set(map_vpi *spm, void* key,
  int value)
{
  map_vpi_storage *contents;

  if (spm->alwaysdefault && ((value) == (spm->defaultvalue)))
    return map_vpi_remove(spm, key);

  contents = &(spm->contents[((mapkit_hash_t)key) % spm->size]);

  if (contents->state == MAPKIT_FREESLOT)
  {
    contents->state = MAPKIT_FULLSLOT;
    contents->key = key;
    contents->value = value;
#ifdef MAPKIT_COLLISIONS
    spm->insertionindexs ++;
#endif
    spm->used ++;
    spm->fill ++;
    if (spm->fill > spm->maxfill)
    {
#ifdef MAPKIT_DEBUG
      fprintf(stderr, "MAPKIT: fill > maxfill\n");
#endif
      return map_vpi_reallocate(spm, map_vpi_growsize(spm, spm->used));
    }
    return MAPKIT_OK;
  }
  else if ((contents->state == MAPKIT_FULLSLOT)
    && ((contents->key) == (key)))
  {
    contents->value = value;
#ifdef MAPKIT_COLLISIONS
    spm->insertionindexs ++;
#endif
    return MAPKIT_OK;
  }
  else
    return map_vpi_set_s(spm, key, value);
}

int map_vpi_value(map_vpi *spm, void* key)
{
  map_vpi_storage *contents;
    
  contents = &(spm->contents[((mapkit_hash_t)key) % spm->size]);

  if ((contents->state == MAPKIT_FULLSLOT)
    && ((contents->key) == (key)))
  {
#ifdef MAPKIT_COLLISIONS
    spm->keyindexs ++;
#endif
    return contents->value;
  }
  else if (spm->alwaysdefault && (contents->state == MAPKIT_FREESLOT))
  {
#ifdef MAPKIT_COLLISIONS
    spm->keyindexs ++;
#endif
    return spm->defaultvalue;
  }
  else
    return map_vpi_value_s(spm, key);
}

mapkit_error map_vpi_get(map_vpi *spm, void* key,
  int *value)
{
  map_vpi_storage *contents;
    
  contents = &(spm->contents[((mapkit_hash_t)key) % spm->size]);

  if ((contents->state == MAPKIT_FULLSLOT)
    && ((contents->key) == (key)))
  {
#ifdef MAPKIT_COLLISIONS
    spm->keyindexs ++;
#endif
    *value = contents->value;
  }
  else if (spm->alwaysdefault && (contents->state == MAPKIT_FREESLOT))
  {
#ifdef MAPKIT_COLLISIONS
    spm->keyindexs ++;
#endif
    *value = spm->defaultvalue;
  }
  else
    return map_vpi_get_s(spm, key, value);
  
  return MAPKIT_OK;
}

int *map_vpi_insertptr(map_vpi *spm, void* key)
{
  map_vpi_storage *contents;

  contents = &(spm->contents[((mapkit_hash_t)key) % spm->size]);

  if (contents->state == MAPKIT_FREESLOT)
  {
    if (spm->fill >= spm->maxfill)
      return map_vpi_insertptr_s(spm, key);

    contents->state = MAPKIT_FULLSLOT;
    contents->key = key;
    contents->value = spm->defaultvalue;
#ifdef MAPKIT_COLLISIONS
    spm->insertionindexs ++;
#endif
    spm->used ++;
    spm->fill ++;
    return &(contents->value);
  }
  else if ((contents->state == MAPKIT_FULLSLOT)
    && ((contents->key) == (key)))
  {
#ifdef MAPKIT_COLLISIONS
    spm->insertionindexs ++;
#endif
    return &(contents->value);
  }
  else
    return map_vpi_insertptr_s(spm, key);
}

int *map_vpi_ptr(map_vpi *spm, void* key)
{
  map_vpi_storage *contents;

  contents = &(spm->contents[((mapkit_hash_t)key) % spm->size]);

  if ((contents->state == MAPKIT_FULLSLOT)
    && ((contents->key) == (key)))
  {
#ifdef MAPKIT_COLLISIONS
    spm->insertionindexs ++;
#endif
    if ((! spm->alwaysdefault) ||
      (! ((contents->value) == (spm->defaultvalue))))
      return &(contents->value);
    else
      return NULL;
  }
  else if (contents->state == MAPKIT_FREESLOT)
  {
#ifdef MAPKIT_COLLISIONS
    spm->insertionindexs ++;
#endif
    return NULL;
  }
  else
    return map_vpi_ptr_s(spm, key);
}


#if defined(__cplusplus)
}
#endif

#endif /* _MAPKIT_VPI_ */
