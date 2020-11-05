#ifndef LIBRETRO_GLUE_H
#define LIBRETRO_GLUE_H

/* zlib */
#include "deps/libz/zlib.h"
#include "deps/libz/unzip.h"
void zip_uncompress(char *in, char *out, char *lastfile);

/* NIBTOOLS */
int nib_convert(char *in, char *out);

/* Misc */
int qstrcmp(const void *a, const void *b);
void remove_recurse(const char *path);

/* String helpers functions */
char* trimwhitespace(char *str);
char* strleft(const char* str, int len);
char* strright(const char* str, int len);
bool strstartswith(const char* str, const char* start);
bool strendswith(const char* str, const char* end);
char *path_remove_program(char *path);
void path_join(char* out, const char* basedir, const char* filename);
char* path_join_dup(const char* basedir, const char* filename);

/* VICE helpers */
extern int log_resources_set_int(const char *name, int value);

#endif /* LIBRETRO_GLUE_H */
