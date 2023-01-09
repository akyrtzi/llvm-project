#ifndef COMPILERCASPLUGIN_CAPI_H
#define COMPILERCASPLUGIN_CAPI_H

#include <stdbool.h>
#include <stddef.h>
#include <stdint.h>

typedef void *llcasplug_cas_options_t;
typedef void *llcasplug_cas_t;

typedef struct {
  const uint8_t *data;
  size_t size;
} llcasplug_digest_t;

typedef struct {
  const void *data;
  size_t size;
} llcasplug_data_t;

typedef struct {
  uint64_t opaque;
} llcasplug_objectid_t;

typedef struct {
  uint64_t opaque;
} llcasplug_loaded_object_t;

typedef struct {
  uint64_t opaque;
} llcasplug_refs_iterator_t;

typedef struct {
  llcasplug_refs_iterator_t begin;
  llcasplug_refs_iterator_t end;
} llcasplug_refs_range_t;

typedef enum {
  LLCASPLUG_LOAD_RESULT_SUCCESS = 0,
  LLCASPLUG_LOAD_RESULT_NOTFOUND = 1,
  LLCASPLUG_LOAD_RESULT_ERROR = 2,
} llcasplug_load_result_t;

typedef struct {
  const char *name;
  llcasplug_objectid_t objid;
} llcasplug_map_entry;

typedef void (*llcasplug_map_visitor)(void *ctx, llcasplug_map_entry);

typedef struct {
  unsigned (*digest_parse)(const char *printed_digest, uint8_t *bytes,
                           size_t bytes_size, char **error);

  bool (*digest_print)(llcasplug_digest_t, char **printed_id, char **error);

  llcasplug_cas_options_t (*cas_options_create)(void);

  void (*cas_options_dispose)(llcasplug_cas_options_t);

  bool (*cas_options_parse)(llcasplug_cas_options_t, const char **opts,
                            size_t opts_count, char **error);

  llcasplug_cas_t (*cas_create)(llcasplug_cas_options_t, char **error);

  void (*cas_dispose)(llcasplug_cas_t);

  void (*string_dispose)(char *);

  bool (*cas_get_objectid)(llcasplug_cas_t, llcasplug_digest_t,
                           llcasplug_objectid_t *, char **error);

  llcasplug_digest_t (*objectid_get_digest)(llcasplug_cas_t,
                                            llcasplug_objectid_t);

  char *(*objectid_print_digest)(llcasplug_cas_t, llcasplug_objectid_t);

  llcasplug_load_result_t (*cas_load_object)(llcasplug_cas_t,
                                             llcasplug_objectid_t,
                                             llcasplug_loaded_object_t *,
                                             char **error);

  bool (*cas_store_object)(llcasplug_cas_t, llcasplug_data_t,
                           const llcasplug_objectid_t *refs, size_t refs_count,
                           llcasplug_objectid_t *, char **error);

  llcasplug_data_t (*loaded_object_get_data)(llcasplug_cas_t,
                                             llcasplug_loaded_object_t);

  llcasplug_refs_range_t (*loaded_object_get_refs_range)(
      llcasplug_cas_t, llcasplug_loaded_object_t);

  llcasplug_refs_iterator_t (*refs_iterator_offset)(llcasplug_cas_t,
                                                    llcasplug_refs_iterator_t,
                                                    ptrdiff_t offset);

  ptrdiff_t (*refs_iterator_distance)(llcasplug_cas_t,
                                      llcasplug_refs_iterator_t begin,
                                      llcasplug_refs_iterator_t end);

  llcasplug_objectid_t (*refs_iterator_get_id)(llcasplug_cas_t,
                                               llcasplug_refs_iterator_t);

  /*===--------------------------------------------------------------------===*\
  |* Action cache API
  \*===--------------------------------------------------------------------===*/

  llcasplug_load_result_t (*actioncache_get_for_digest)(
      llcasplug_cas_t, llcasplug_digest_t key, llcasplug_objectid_t *p_value,
      bool upstream, char **error);

  bool (*actioncache_put_for_digest)(llcasplug_cas_t, llcasplug_digest_t key,
                                     llcasplug_objectid_t value, bool upstream,
                                     char **error);

  llcasplug_load_result_t (*actioncache_get_map_for_digest)(
      llcasplug_cas_t, llcasplug_digest_t key, void *ctx, llcasplug_map_visitor,
      bool upstream, char **error);

  bool (*actioncache_put_map_for_digest)(llcasplug_cas_t,
                                         llcasplug_digest_t key,
                                         const llcasplug_map_entry *entries,
                                         size_t entries_count, bool upstream,
                                         char **error);

} llcasplug_functions_t;

#endif /* COMPILERCASPLUGIN_CAPI_H */
