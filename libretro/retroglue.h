#ifndef _RETRO_GLUE_H_
#define _RETRO_GLUE_H_

#include "deps/libz/zlib.h"
#include "deps/libz/unzip.h"
void zip_uncompress(char *in, char *out, char *lastfile);

int nib_convert(char *in, char *out);

void remove_recurse(const char *path);
extern int log_resources_set_int(const char *name, int value);
#endif
