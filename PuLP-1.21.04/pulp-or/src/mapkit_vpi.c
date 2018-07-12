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

static char const rcsid[] =
  "@(#) $Jeannot: mapkit_vpi.c,v 1.4 2005/05/05 09:23:51 js Exp $";

#include <stdlib.h>
#include <stdio.h>
#include <limits.h>

#include "mapkit_vpi.h"
/*
  MapKit
  Copyright (c) 2002-2005, Jean-Sebastien Roy (js@jeannot.org)
  @(#) $Jeannot: mapkit_vpi.c,v 1.4 2005/05/05 09:23:51 js Exp $
*/

/* @(#) $Jeannot: mapkit_vpi.c,v 1.4 2005/05/05 09:23:51 js Exp $ */


/* Map structures */

#ifdef MAPKIT_map_vpi

/*
  Implementation for map_vpi (void* -> int)
  Default value : 0
  Uses a state field.
*/

/* Static prototypes */

/* Return the index of key, or MAPKIT_KEYNOTFOUND */
static mapkit_size_t map_vpi_keyindex(map_vpi *spm, void* key);

/* Return the index of key or -(insertion index)-1 if key not found */
static mapkit_size_t map_vpi_insertionindex(map_vpi *spm, void* key);

/* Implementation */

mapkit_error map_vpi_init(map_vpi *spm)
{
  return map_vpi_init_hint(spm, MAPKIT_DEFAULT_EXPECTEDUSED);
}

mapkit_error map_vpi_init_hint(map_vpi *spm, mapkit_size_t used)
{
#ifdef MAPKIT_DEBUG
  fprintf(stderr, "MAPKIT: init\n");
#endif

  spm->size = 0;
  spm->fill = 0;
  spm->used = 0;
  spm->maxfillfactor = 0.5;
  spm->minusedfactor = 0.2;
  spm->contents = NULL;
#ifdef MAPKIT_COLLISIONS
  spm->insertionindexs = spm->insertionindex_collisions = 0;
  spm->keyindexs = spm->keyindex_collisions = 0;
#endif
  spm->defaultvalue = 0;
  spm->alwaysdefault = 0;

  return map_vpi_reallocate(spm, map_vpi_meansize(spm, used));
}

mapkit_error map_vpi_ensurecapacity(map_vpi *spm, mapkit_size_t used)
{
  if (used > (spm->used + spm->maxfill - spm->fill))
  {
#ifdef MAPKIT_DEBUG
    fprintf(stderr, "MAPKIT: need more capacity\n");
#endif
    return map_vpi_reallocate(spm, map_vpi_meansize(spm, used));
  }
  else
    return MAPKIT_OK;
}

mapkit_error map_vpi_adjustcapacity(map_vpi *spm)
{
  spm->minused = (mapkit_size_t) (spm->size*spm->minusedfactor);
  spm->maxfill = (mapkit_size_t) (spm->size*spm->maxfillfactor);
  
  if (spm->used < spm->minused)
  {
#ifdef MAPKIT_DEBUG
    fprintf(stderr, "MAPKIT: used < minused\n");
#endif
    return map_vpi_reallocate(spm, map_vpi_meansize(spm, spm->used));
  }
  else if (spm->fill > spm->maxfill)
  {
#ifdef MAPKIT_DEBUG
    fprintf(stderr, "MAPKIT: fill > maxfill\n");
#endif
    return map_vpi_reallocate(spm, map_vpi_meansize(spm, spm->used));
  }
  else
    return MAPKIT_OK;
}

void map_vpi_free(map_vpi *spm)
{
  free(spm->contents);
  spm->contents = NULL;
  spm->size = 0;
  spm->fill = 0;
  spm->used = 0;
  spm->maxfill = 0;
  spm->minused = 0;
#ifdef MAPKIT_COLLISIONS
  spm->insertionindexs = spm->insertionindex_collisions = 0;
  spm->keyindexs = spm->keyindex_collisions = 0;
#endif
}

mapkit_error map_vpi_copy(map_vpi *to, map_vpi *from)
{
  map_vpi_storage *contentscopy;
  size_t size = from->size * sizeof(*from->contents);

  contentscopy = malloc(size);
  if (contentscopy == NULL)
    MAPKIT_ERROR(MAPKIT_ENOMEM);
    
  memcpy(to, from, sizeof(map_vpi));
  to->contents = contentscopy;
  memcpy(to->contents, from->contents, size);

  return MAPKIT_OK;
}

mapkit_size_t map_vpi_growsize(map_vpi *spm, mapkit_size_t used)
{
  return (mapkit_size_t)(4.0 * used / (3*spm->minusedfactor + spm->maxfillfactor));
}

mapkit_size_t map_vpi_shrinksize(map_vpi *spm, mapkit_size_t used)
{
  return (mapkit_size_t)(4.0 * used / (spm->minusedfactor + 3*spm->maxfillfactor));
}

mapkit_size_t map_vpi_meansize(map_vpi *spm, mapkit_size_t used)
{
  return (mapkit_size_t)(2.0 * used / (spm->minusedfactor + spm->maxfillfactor));
}

mapkit_error map_vpi_reallocate(map_vpi *spm, mapkit_size_t newsize)
{
  mapkit_size_t index;
  mapkit_size_t oldsize;
  map_vpi_storage *newcontents, *oldcontents;

  /* At least one free entry */
  if (newsize <= spm->used) newsize = spm->used + 1;
  newsize = mapkit_nextprime(newsize);
  if (newsize <= spm->used)
    MAPKIT_ERROR(MAPKIT_ETOOBIG);

  newcontents = malloc(newsize * sizeof(*spm->contents));
  if (newcontents == NULL)
    MAPKIT_ERROR(MAPKIT_ENOMEM);

  /* Initialize all entries to "free" */
  for (index = 0 ; index < newsize ; index ++)
    newcontents[index].state = MAPKIT_FREESLOT;

  oldcontents = spm->contents;
  oldsize = spm->size;
  spm->contents = newcontents;
  spm->size = newsize;

#ifdef MAPKIT_DEBUG
  fprintf(stderr, "MAPKIT: reallocate %ld -> %ld\n", (long)oldsize,
    (long)newsize);
#endif

  spm->maxfill = (mapkit_size_t) (newsize*spm->maxfillfactor);
  /* At least one free entry */
  if (spm->maxfill >= newsize)
    spm->maxfill = newsize - 1;
  spm->minused = (mapkit_size_t) (newsize*spm->minusedfactor);
  spm->used = 0;


  if (oldcontents != NULL)
  {
    int used = 0;
    void* key;
    int defaultvalue = spm->defaultvalue;
    int notalwaysdefault = ! spm->alwaysdefault;

    /* Copy all entries from old to new */
    for (index = 0 ; index < oldsize ; index ++)
      if (oldcontents[index].state == MAPKIT_FULLSLOT)
      {
        mapkit_size_t ins_index;
        map_vpi_storage *contents;

        key = oldcontents[index].key;

        /* Fast path */
        ins_index = ((mapkit_hash_t)key) % spm->size;
        contents = &(newcontents[ins_index]);

        if (contents->state != MAPKIT_FREESLOT)
        {
          ins_index = map_vpi_insertionindex(spm, key);
          contents = &(newcontents[ins_index]);
        }
#ifdef MAPKIT_COLLISIONS
        else
          spm->insertionindexs ++;
#endif
        if (notalwaysdefault ||
          (! ((oldcontents[index].value) == (defaultvalue))))
        {
          contents->value = oldcontents[index].value;
          contents->state = MAPKIT_FULLSLOT;
          contents->key = key;
          used ++;
        }
      }
    free(oldcontents);
    spm->used = used;
  }
  spm->fill = spm->used;

  return MAPKIT_OK;
}

int map_vpi_value_s(map_vpi *spm, void* key)
{
  mapkit_size_t index;
  
  index = map_vpi_keyindex(spm, key);

  if (index < 0)
  {
    if (spm->alwaysdefault)
      return spm->defaultvalue;
    else
      MAPKIT_FATAL_ERROR(MAPKIT_EKEYNOTFOUND);
  }

  return spm->contents[index].value;
}

mapkit_error map_vpi_get_s(map_vpi *spm, void* key,
  int *value)
{
  mapkit_size_t index;
  
  index = map_vpi_keyindex(spm, key);

  if (index < 0)
  {
    if (spm->alwaysdefault)
      *value = spm->defaultvalue;
    else
      MAPKIT_ERROR(MAPKIT_EKEYNOTFOUND);
  }

  *value = spm->contents[index].value;
  return MAPKIT_OK;
}

mapkit_error map_vpi_set_s(map_vpi *spm, void* key,
  int value)
{
  mapkit_size_t index;

  index = map_vpi_insertionindex(spm, key);

  if (index < 0)
    /* FULLSLOT */
    spm->contents[-index-1].value = value;
  else
  {
    map_vpi_storage *element = &(spm->contents[index]);
    int free = element->state == MAPKIT_FREESLOT;
    element->state = MAPKIT_FULLSLOT;
    element->value = value;
    element->key = key;
    spm->used ++;
    
    if (free && ((++ spm->fill) > spm->maxfill))
    {
#ifdef MAPKIT_DEBUG
      fprintf(stderr, "MAPKIT: fill > maxfill\n");
#endif
      return map_vpi_reallocate(spm, map_vpi_growsize(spm, spm->used));
    }
  }
  return MAPKIT_OK;
}

int *map_vpi_insertptr_s(map_vpi *spm, void* key)
{
  mapkit_size_t index;

  index = map_vpi_insertionindex(spm, key);

  if (index < 0)
    return &(spm->contents[-index-1].value);
  else
  {
    map_vpi_storage *element = &(spm->contents[index]);
    if (element->state == MAPKIT_FREESLOT);
    {
      /* FREESLOT */
      if (spm->fill >= spm->maxfill)
      {
        mapkit_error err;
#ifdef MAPKIT_DEBUG
        fprintf(stderr, "MAPKIT: fill => maxfill before insert\n");
#endif
        /* Must reallocate -before- inserting defaultvalue */
        err = map_vpi_reallocate(spm, map_vpi_growsize(spm, spm->used + 1));
        if (err)
        {
          MAPKIT_ERROR_NORET(err);
          return NULL;
        }
        
        index = map_vpi_insertionindex(spm, key);
        /* FREESLOT */
        spm->contents[index].state = MAPKIT_FULLSLOT;
        spm->contents[index].key = key;
        spm->contents[index].value = spm->defaultvalue;
        spm->used ++;
        spm->fill ++;
        return &(spm->contents[index].value);
      }
      else
        spm->fill ++;
    }

    element->state = MAPKIT_FULLSLOT;
    element->key = key;
    element->value = spm->defaultvalue;
    spm->used ++;
    return &(element->value);
  }
}

int *map_vpi_ptr_s(map_vpi *spm, void* key)
{
  mapkit_size_t index;

  index = map_vpi_keyindex(spm, key);

  if ((index >= 0) &&
    ((! spm->alwaysdefault) ||
      (! ((spm->contents[index].value) == (spm->defaultvalue)))))
    return &(spm->contents[index].value);
  else
    return NULL;
}

mapkit_error map_vpi_remove_s(map_vpi *spm, void* key)
{
  mapkit_size_t index;

  index = map_vpi_keyindex(spm, key);

  if (index < 0)
  {
    if (spm->alwaysdefault)
      return MAPKIT_OK;
    else
      return MAPKIT_EKEYNOTFOUND;
  }

  spm->contents[index].state = MAPKIT_DELETEDSLOT;
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

mapkit_size_t map_vpi_keyindex(map_vpi *spm, void* key)
{
  mapkit_size_t index, decrement;

  signed char state;
  
  index = ((mapkit_hash_t)key) % spm->size;
  decrement = (((mapkit_hash_t)key) % (spm->size-2));
  decrement += (decrement == 0);

#ifdef MAPKIT_COLLISIONS
  spm->keyindexs ++;
#endif
  
  while ((state = spm->contents[index].state) != MAPKIT_FREESLOT
    && (state == MAPKIT_DELETEDSLOT ||
      (! ((spm->contents[index].key) == (key)))))
  {
#ifdef MAPKIT_COLLISIONS
    spm->keyindex_collisions ++;
#endif
    index -= decrement;
    if (index < 0) index += spm->size;
  }

  if (state == MAPKIT_FREESLOT) return MAPKIT_KEYNOTFOUND;
  return index;
}

mapkit_size_t map_vpi_insertionindex(map_vpi *spm, void* key)
{
  mapkit_size_t index, decrement;
  signed char state;

#ifdef MAPKIT_COLLISIONS
  spm->insertionindexs ++;
#endif
  
  index = ((mapkit_hash_t)key) % spm->size;

  /* Fast path (largely superfluous) */
  if ((state = spm->contents[index].state) == MAPKIT_FREESLOT) return index;
  if ((state == MAPKIT_FULLSLOT)
    && ((spm->contents[index].key) == (key))) return -index-1;

  decrement = (((mapkit_hash_t)key) % (spm->size-2));
  decrement += (decrement == 0);
  
  while ((state == MAPKIT_FULLSLOT)
    && (! ((spm->contents[index].key) == (key))))
  {
#ifdef MAPKIT_COLLISIONS
    spm->insertionindex_collisions ++;
#endif
    index -= decrement;
    if (index < 0) index += spm->size;
    state = spm->contents[index].state;
  }

  if (state == MAPKIT_DELETEDSLOT)
  {
    mapkit_size_t index2 = index;
    while ((state = spm->contents[index].state) != MAPKIT_FREESLOT
      && ((state == MAPKIT_DELETEDSLOT)
        || (! ((spm->contents[index].key) == (key)))))
    {
      index -= decrement;
      if (index < 0) index += spm->size;
      state = spm->contents[index].state;
    }
    if (state == MAPKIT_FREESLOT) return index2;
  }

  if (state == MAPKIT_FULLSLOT) return -index-1;
  return index;
}

mapkit_size_t map_vpi_next(map_vpi *spm, mapkit_size_t index)
{
  mapkit_size_t size = spm->size;
  map_vpi_storage *pos_contents;
  int defaultvalue = spm->defaultvalue;
  int notalwaysdefault = ! spm->alwaysdefault;

  pos_contents = &(spm->contents[++index]);
  
  for ( ; index < size ; index ++, pos_contents ++)
    if ((pos_contents->state == MAPKIT_FULLSLOT)
      && (notalwaysdefault ||
        (! ((pos_contents->value) == (defaultvalue)))))
      return index;

  return -1;
}

map_vpi_storage *map_vpi_nextptr(map_vpi *spm, map_vpi_storage *pos_contents)
{
  map_vpi_storage *end = &(spm->contents[spm->size]);
  int defaultvalue = spm->defaultvalue;
  int notalwaysdefault = ! spm->alwaysdefault;
  
  if (pos_contents == NULL)
    pos_contents = spm->contents;
  else
  {
    pos_contents ++;
    if (pos_contents <= spm->contents)
      return NULL;
  }

  for ( ; pos_contents < end ; pos_contents ++)
    if ((pos_contents->state == MAPKIT_FULLSLOT)
      && (notalwaysdefault ||
        (! ((pos_contents->value) == (defaultvalue)))))
      return pos_contents;

  return NULL;
}

mapkit_error map_vpi_getall(map_vpi *spm, map_vpi_element **array, mapkit_size_t *count)
{
  mapkit_size_t index;
  mapkit_size_t size = spm->size, vcount = 0;
  map_vpi_element *pos_array;
  map_vpi_storage *pos_contents;
  int defaultvalue = spm->defaultvalue;
  int notalwaysdefault = ! spm->alwaysdefault;
  
  pos_array = *array = malloc(sizeof(**array)*spm->used);
  if (*array == NULL)
    MAPKIT_ERROR(MAPKIT_ENOMEM);
  
  pos_contents = spm->contents;

  for (index = 0 ; index < size ; index ++, pos_contents ++)
    if ((pos_contents->state == MAPKIT_FULLSLOT)
      && (notalwaysdefault ||
        (! ((pos_contents->value) == (defaultvalue)))))
    {
      pos_array->key = pos_contents->key;
      pos_array->value = pos_contents->value;
      pos_array ++;
      vcount ++;
    }
  *count = vcount;

  return MAPKIT_OK;
}

mapkit_error map_vpi_clean(map_vpi *spm)
{
  mapkit_size_t index, count = 0;
  mapkit_size_t size = spm->size;
  map_vpi_storage *pos_contents;
  int defaultvalue = spm->defaultvalue;
    
  pos_contents = spm->contents;
  for (index = 0 ; index < size ; index ++, pos_contents ++)
    if ((pos_contents->state == MAPKIT_FULLSLOT)
      && ((pos_contents->value) == (defaultvalue)))
    {
      pos_contents->state = MAPKIT_DELETEDSLOT;
      count ++;
    }

  spm->used -= count;
  if (spm->used < spm->minused)
  {
#ifdef MAPKIT_DEBUG
    fprintf(stderr, "MAPKIT: used < minused\n");
#endif
    return map_vpi_reallocate(spm, map_vpi_meansize(spm, spm->used));
  }

  return MAPKIT_OK;
}

int map_vpi_compare(const void *e1, const void *e2)
{
  void* key1 = ((map_vpi_element *)e1)->key;
  void* key2 = ((map_vpi_element *)e2)->key;

  return ((key1)<(key2) ? -1 : ((key1) == (key2) ? 0 : 1));
}

mapkit_error map_vpi_getall_sorted(map_vpi *spm, map_vpi_element **array,
  mapkit_size_t *count)
{
  mapkit_error err;
  
  err = map_vpi_getall(spm, array, count);
  if (err)
    MAPKIT_ERROR(err);
  
  qsort(*array, *count, sizeof(**array), map_vpi_compare);

  return MAPKIT_OK;
}

mapkit_error map_vpi_setall(map_vpi *spm, map_vpi_element *array, mapkit_size_t count)
{
  mapkit_size_t array_index;
  mapkit_error err;
    
  err = map_vpi_ensurecapacity(spm, spm->used + count);
  if (err)
    MAPKIT_ERROR(err);

  if (spm->alwaysdefault)
    /* Prevent shrinking */
    spm->minused = 0;
  
  for (array_index = 0 ; array_index < count ; array_index ++)
  {
    err = map_vpi_set(spm, array[array_index].key, array[array_index].value);
    if (err)
      MAPKIT_ERROR(err);
  }

  if (spm->alwaysdefault)
    map_vpi_adjustcapacity(spm);

  return MAPKIT_OK;
}

mapkit_error map_vpi_removeall(map_vpi *spm, void* *array,
  mapkit_size_t count)
{
  mapkit_size_t array_index;
  mapkit_error err;

  /* Prevent shrinking */
  spm->minused = 0;
  
  for (array_index = 0 ; array_index < count ; array_index ++)
  {
    err = map_vpi_remove(spm, array[array_index]);
    if (err)
      MAPKIT_ERROR(err);
  }

  map_vpi_adjustcapacity(spm);

  return MAPKIT_OK;
}

void map_vpi_printstats(map_vpi *spm)
{
  fprintf(stderr, "MAPKIT: map_vpi statistics\n");
  fprintf(stderr, "MAPKIT: alwaysdefault = %d\n", spm->alwaysdefault);
  fprintf(stderr,
    "MAPKIT: minused = %ld, maxfill = %ld\n",
    (long)spm->minused, (long)spm->maxfill);
  fprintf(stderr,
    "MAPKIT: minusedfactor = %g, maxfillfactor = %g\n",
    spm->minusedfactor, spm->maxfillfactor);
#ifdef MAPKIT_COLLISIONS
  fprintf(stderr,
    "MAPKIT: insertionindexs = %lu, collisions = %lu\n",
    (unsigned long)spm->insertionindexs,
    (unsigned long)spm->insertionindex_collisions);
  fprintf(stderr,
    "MAPKIT: keyindexs = %lu, collisions = %lu\n",
    (unsigned long)spm->keyindexs, (unsigned long)spm->keyindex_collisions);
#endif
}

#endif /* MAPKIT_map_vpi */


