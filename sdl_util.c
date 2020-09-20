#include "sdl_util.h"
#define STB_IMAGE_IMPLEMENTATION
#include "stb_image.h"
TextureResult load_texture(char *file_name, SDL_Renderer *renderer)
{
    //load a 32-bit, rgba texture
    TextureResult texture_result;
    texture_result.loaded = false;
    texture_result.image = stbi_load(file_name, &texture_result.im_width, &texture_result.im_height, &texture_result.im_channels, 0);
    if (texture_result.image == NULL) {
	//error handling
	return texture_result;
    }
    SDL_Surface *img_surface = SDL_CreateRGBSurfaceWithFormatFrom((void*)texture_result.image, texture_result.im_width, texture_result.im_height, 32, 4*texture_result.im_width, SDL_PIXELFORMAT_RGBA32);
    if (img_surface == NULL) {
	//error handling
	return texture_result;
    }
    texture_result.texture = SDL_CreateTextureFromSurface(renderer, img_surface);
    SDL_FreeSurface(img_surface);
    if (texture_result.texture == NULL) {
	//error handling
	return texture_result;
    }    
    texture_result.loaded = true;
    return texture_result;
}
