

#ifndef _SDL_UTIL_H_
#include "SDL.h"
#include <stdio.h>
#include <math.h>
#include <stdbool.h>
#include <stdlib.h>
#include "SDL.h"
#include "gmo_types.h"


typedef struct {
    byte *image;
    SDL_Texture *texture;
    int32 im_width;
    int32 im_height;
    int32 im_channels;
    bool loaded;
} TextureResult;

TextureResult load_texture(char *file_name, SDL_Renderer *renderer);

#define _SDL_UTIL_H_
#endif
