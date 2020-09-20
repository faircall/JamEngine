#define _CRT_SECURE_NO_WARNINGS
#include <stdio.h>
#include <math.h>
#include <stdbool.h>
#include <stdlib.h>
#include "SDL.h"
#include "gmo_types.h"
#include "gmo_math.h"
#include "gmo_sound.h"

#include "sdl_util.h"

#include <windows.h>
#define SCREENWIDTH 1280
#define SCREENHEIGHT 720




/*
 * Todo: Some basic editor functionality would be
 * Creating an array of collision rectangles, and saving them to disk
 * The saving to disk part is somewhat difficult if we want to
 * be able to list directory contents. That *is* operating system specific I believe.
 *
 * I *think* I need to pass some info from argv into my editor function, to use
 * FindFirstFile for example
 * */

typedef enum {
    BRAIN_RESTING,
    BRAIN_TRACKING,
    BRAIN_ATTACKING,
    BRAIN_DYING,
    BRAIN_DEAD
} BRAIN_STATE;

typedef enum {
    GAME_TITLE,
    GAME_PLAYING,
    GAME_OVER,
    GAME_EDITOR
} GAME_STATE;

typedef struct {
    int32 id;
    vec2 position;
    vec2 heading;
    vec2 last_heading;
    vec2 velocity;
    vec2 acceleration;
    real32 acceleration_value;
    real32 max_speed;
    bool attacking;
    //store info on arms, legs
    bool exists;
    //health etc
    real32 health;
    real32 attack_timer;
    SDL_Rect rect;
    BRAIN_STATE brain_state;
} Entity;

typedef struct {
    GAME_STATE current_state;
    int32 current_score;
    int32 high_score;
} GameState;

typedef struct {
    SDL_Rect original_rect;
    SDL_Rect potential_rect;    
} RectToSave;




Entity entity_init(int32 id, vec2 position, vec2 heading, vec2 last_heading, vec2 velocity, vec2 acceleration, real32 acceleration_value, real32 max_speed, bool attacking, real32 health, SDL_Rect rect, BRAIN_STATE brain_state);

SDL_Rect rect_init(real32 x, real32 y, real32 w, real32 h);
RectToSave rect_to_save_init(real32 x, real32 y, real32 w, real32 h);
bool collides(SDL_Rect object_rect, SDL_Rect geometry_rect);


void do_title(GameState *game_state, ALuint al_source, SDL_Renderer *sdl_renderer, TextureResult title_texture, bool *g_running);

void editor_update_and_render(SDL_Renderer *sdl_renderer, SDL_Rect **editor_rect_list, uint32 *num_rects, uint32 *rect_storage, int32 *mouse_x, int32 *mouse_y, bool *g_running);

void game_update_and_render(SDL_Renderer *sdl_renderer, Entity *player, bool *g_running, int32 *mouse_x, int32 *mouse_y);

void strip_newline(char *string);
void get_string_stripped(char *string);

int main(int argc, char **argv)
{
    /* SDL Init */
    if (SDL_Init(SDL_INIT_VIDEO | SDL_INIT_GAMECONTROLLER) != 0) {
        SDL_Log("Didn't init: %s\n", SDL_GetError());
    }
    
    SDL_Window *sdl_window;
    SDL_Renderer *sdl_renderer;
    
    if (SDL_CreateWindowAndRenderer(SCREENWIDTH, SCREENHEIGHT, SDL_WINDOW_RESIZABLE, &sdl_window, &sdl_renderer) != 0) {
        SDL_Log("Didn't create window: %s\n", SDL_GetError());
    }
    
    /* OpenAL init */
    ALCdevice *al_device;
    alGetError();
    al_device = alcOpenDevice(NULL);
    if (check_error()) {
        printf("error opening al device\n");
    }
    if (!al_device) {
        printf("didn't open al device\n");
    }    
    ALCcontext *al_context;
    al_context = alcCreateContext(al_device, NULL);
    if (check_error()) {
        printf("error opening al context\n");
    }
    if (!alcMakeContextCurrent(al_context)) {
        printf("couldn't make Al context current\n");
    }
    
    /* load some images */

    TextureResult title_texture = load_texture("art/gmo.png", sdl_renderer);
    
    
    /* load sounds*/

    //wrap this
    ALfloat listener_orientation[] = {0.0f, 0.0f, 0.0f, 0.0f, -1.0f, 0.0f};     
    alListener3f(AL_POSITION, 0.0f, 0.0f, 0.0f);//this could be updated according to player pos
    alListener3f(AL_VELOCITY, 0.0f, 0.0f, 0.0f);//this could be updated according to player velocity
    alListenerfv(AL_ORIENTATION, listener_orientation);

    //wrap this
    ALuint elitch_buffer = al_buffer_from_wav("sounds/ELITCH.wav");

    ALint al_source_state;

    ALuint al_source;
    vec3 pos = vec3_init(0.0f, 0.0f, 0.0f);

    al_make_source(al_context, &al_source, elitch_buffer, pos, pos, 1.0f, 1.0f, AL_FALSE);

    /* set some globals */
    bool g_running = true;
    real32 dt, total_time_ms = 0.0f;
    uint32 last_time = 0, current_time = 0;

    int32 mouse_x, mouse_y;
    mouse_x = mouse_y = 0;

    GameState game_state;
    game_state.current_state = GAME_EDITOR;
    game_state.current_score = 0;
    game_state.high_score = 0;
    /* eventually create arrays for entities and maybe geometry */

    uint32 num_rects = 0;
    uint32 rect_storage = 10;
    SDL_Rect *editor_rect_list = (SDL_Rect*)malloc(sizeof(SDL_Rect) * rect_storage);

    
    Entity player = entity_init(1, vec2_init(40.0f, 40.0f), vec2_init(0.0f, 0.0f),
				vec2_init(0.0f, 0.0f), vec2_init(0.0f, 0.0f), vec2_init(0.0f, 0.0f),
				3000.0f, 700.0f, false, 100.0f, rect_init(40.0f, 40.0f, 32.0f, 32.0f), BRAIN_RESTING);
    
    while (g_running) {
	SDL_Event sdl_event;
	if (game_state.current_state == GAME_TITLE) {
	    do_title(&game_state, al_source, sdl_renderer, title_texture, &g_running);	    
	} else if (game_state.current_state == GAME_OVER) {
	    while (SDL_PollEvent(&sdl_event)) {
		if (sdl_event.type == SDL_QUIT) {
		    g_running = false;
		    break;
		}
	    }
	    //render gameover
	} else if (game_state.current_state == GAME_PLAYING) {	    
	    game_update_and_render(sdl_renderer, &player, &g_running, &mouse_x, &mouse_y);	    
	} else if (game_state.current_state == GAME_EDITOR) {
	    editor_update_and_render(sdl_renderer, &editor_rect_list, &num_rects, &rect_storage, &mouse_x, &mouse_y, &g_running);
	}
	
    }

    SDL_Quit();
    return 0;
}

Entity entity_init(int32 id, vec2 position, vec2 heading, vec2 last_heading, vec2 velocity, vec2 acceleration, real32 acceleration_value, real32 max_speed, bool attacking, real32 health, SDL_Rect rect, BRAIN_STATE brain_state)
{
    Entity result;
    result.id = id;
    result.position = position;
    result.heading = heading;
    result.last_heading = last_heading;
    result.velocity = velocity;
    result.acceleration = acceleration;
    result.acceleration_value = acceleration_value;
    result.max_speed = max_speed;
    result.attacking = attacking;
    result.health = health;
    result.rect = rect;
    result.exists= true;
    result.brain_state= brain_state;
    return result;
}

SDL_Rect rect_init(real32 x, real32 y, real32 w, real32 h)
{
    SDL_Rect result;
    result.x = x;
    result.y = y;
    result.w = w;
    result.h = h;
    return result;
}

RectToSave rect_to_save_init(real32 x, real32 y, real32 w, real32 h)
{
    RectToSave result;
    SDL_Rect current;
    current.x = x;
    current.y = y;
    current.w = w;
    current.h = h;
    result.original_rect = current;
    result.potential_rect= current;
    return result;
}

bool collides(SDL_Rect object_rect, SDL_Rect geometry_rect)
{
    bool result = false;
    real32 radius = object_rect.w/2.0f;
    SDL_Rect minkowski_sum;
    minkowski_sum.x = geometry_rect.x;
    minkowski_sum.y = geometry_rect.y;
    minkowski_sum.w = geometry_rect.w;
    minkowski_sum.h = geometry_rect.h;
    minkowski_sum.w += radius;
    minkowski_sum.h += radius;
    minkowski_sum.x -= radius/2.0f;
    minkowski_sum.y -= radius/2.0f;
    real32 object_center_x = object_rect.x + radius;
    real32 object_center_y = object_rect.y + radius;
    if (object_center_x >= minkowski_sum.x &&
        object_center_x <= (minkowski_sum.x + minkowski_sum.w) &&
        object_center_y >= (minkowski_sum.y) &&
        object_center_y <= (minkowski_sum.y + minkowski_sum.h)) {
        result = true;
    }					   				      
    return result;
}

void do_title(GameState *game_state, ALuint al_source, SDL_Renderer *sdl_renderer, TextureResult title_texture, bool *g_running)
{
    	SDL_Event sdl_event;

	while (SDL_PollEvent(&sdl_event)) {
	    if (sdl_event.type == SDL_QUIT) {
		*g_running = false;
		break;
	    }
	}
	SDL_SetRenderDrawColor(sdl_renderer, 0xbf, 0x00, 0xaf, 0xff);
	SDL_RenderClear(sdl_renderer);
	SDL_RenderCopy(sdl_renderer, title_texture.texture, NULL, NULL);
	SDL_RenderPresent(sdl_renderer);
	//need a test here
	//wrap the below
	ALint al_source_state;
	alGetSourcei(al_source, AL_SOURCE_STATE, &al_source_state);
	//printf("al step source is definitely %d\n", al_step_source);
	if (al_source_state == AL_STOPPED) {
	    //printf("al step source is stopped %d\n", al_source);
	    alSourcePlay(al_source);
	    al_source_state = AL_PLAYING;
	} else if (al_source_state == AL_PLAYING){
	    //printf("al step source is playing %d\n", al_source);
	} else if (al_source_state == AL_INITIAL){
	    //printf("al step source is initial %d\n", al_source);
	    alSourcePlay(al_source);
	} else if (al_source_state == AL_PAUSED){
	    //printf("al step source is pausedd %d\n", al_source);
	}
	    

	//render title

}

void game_update_and_render(SDL_Renderer *sdl_renderer, Entity *player, bool *g_running, int32 *mouse_x, int32 *mouse_y)
{
    SDL_Event sdl_event;
    /* timer stuff */
    uint32 current_time = SDL_GetTicks();
    static last_time = 0;
    real32 dt = ((float)(current_time - last_time))/1000.0f;
    last_time = current_time;
        
    /* reset some flags*/
    player->attacking = false;
    player->heading = vec2_init(0.0f, 0.0f);
    real32 friction_value = 1800.f;
    real32 velocity_epsilon = 1.0f;
    /* event handling */

    while (SDL_PollEvent(&sdl_event)) {
	if (sdl_event.type == SDL_QUIT) {
	    *g_running = false;
	    break;
	}
	if (sdl_event.type == SDL_KEYDOWN) {
	    if (sdl_event.key.keysym.scancode == SDL_SCANCODE_SPACE) {
		player->attacking = true;
	    }
	}
	if (sdl_event.type == SDL_MOUSEMOTION) {
	    SDL_GetMouseState(mouse_x, mouse_y);
	}
	if (sdl_event.type == SDL_MOUSEBUTTONDOWN) {
	    //make a vector from mouse pos
                
	}
    }
        
    /* get key state*/
    const byte *key_state = SDL_GetKeyboardState(NULL);
    if (key_state[SDL_SCANCODE_W]) {
	player->heading.y = -1.0f;
	player->acceleration.y = -player->acceleration_value;
    }
    if (key_state[SDL_SCANCODE_S]) {
	player->heading.y = 1.0f;
	player->acceleration.y = player->acceleration_value;
    }
    if (key_state[SDL_SCANCODE_A]) {
	player->heading.x = -1.0f;
	player->acceleration.x = -player->acceleration_value;
            
    }
    if (key_state[SDL_SCANCODE_D]) {
	player->heading.x = 1.0f;
	player->acceleration.x = player->acceleration_value;	    
    }
        
    if (vec2_mag(player->heading) > 0.0f) {
	player->heading = vec2_normalize(player->heading);
	player->last_heading = player->heading;
    }
    player->acceleration = vec2_scale(player->acceleration_value, player->heading);
    player->velocity.x += player->acceleration.x * dt;
    player->velocity.y += player->acceleration.y * dt;
        
    real32 player_speed = vec2_mag(player->velocity);
    if (player_speed > velocity_epsilon) {
	vec2 friction = vec2_normalize(player->velocity);
	friction = vec2_scale(-friction_value, friction);
	player->velocity.x += friction.x * dt;
	player->velocity.y += friction.y * dt;
    }
        
    player_speed = vec2_mag(player->velocity);
    if (player_speed > player->max_speed) {
	player->velocity = vec2_normalize(player->velocity);
	player->velocity = vec2_scale(player->max_speed, player->velocity);
    }
        
    /* final integration */
    player->position.x += player->velocity.x * dt;
    player->position.y += player->velocity.y * dt;
        
        
    /* update appropriate rects */
    player->rect.x = player->position.x;
    player->rect.y = player->position.y;

        
    SDL_SetRenderDrawColor(sdl_renderer, 0xbf, 0x00, 0xaf, 0xff);
    SDL_RenderClear(sdl_renderer);
    SDL_SetRenderDrawColor(sdl_renderer, 0xff, 0x00, 0x00, 0xff);    
    
    SDL_RenderFillRect(sdl_renderer, &player->rect);
		
        
        
    /* update enemy brain here */

    SDL_RenderPresent(sdl_renderer);
}

void editor_update_and_render(SDL_Renderer *sdl_renderer, SDL_Rect **editor_rect_list, uint32 *num_rects, uint32 *rect_storage, int32 *mouse_x, int32 *mouse_y, bool *g_running)    
{
    SDL_Event sdl_event;
    static bool want_new_rect = false;
    bool created_new_rect = false;
    bool want_save = false;    
    bool want_load = false;
    static RectToSave new_rect;// = rect_to_save_init(0.0f, 0.0f, 20.0f, 20.0f);
    while (SDL_PollEvent(&sdl_event)) {
	if (sdl_event.type == SDL_QUIT) {
	    *g_running = false;
	    break;
	}
	if (sdl_event.type == SDL_KEYUP) {
	    if (sdl_event.key.keysym.scancode == SDL_SCANCODE_S) {
		want_save = true;
	    }
	    if (sdl_event.key.keysym.scancode == SDL_SCANCODE_L) {
		want_load = true;
	    }
	}
	if (sdl_event.type == SDL_MOUSEMOTION) {
	    SDL_GetMouseState(mouse_x, mouse_y);
	}
	if (sdl_event.type == SDL_MOUSEBUTTONDOWN) {
	    if (sdl_event.button.button == SDL_BUTTON_LEFT) {
	    //make a vector from mouse pos
		if (!want_new_rect) {
		    want_new_rect = true;
		    new_rect.original_rect = rect_init(*mouse_x, *mouse_y, 20.0f, 20.0f);
		} else {
		    created_new_rect= true;		    
		}
	    }
	    else if (sdl_event.button.button == SDL_BUTTON_RIGHT) {
		
	    }
	}
    }

    if (want_new_rect) {

	if (created_new_rect) {
	    (*editor_rect_list)[*num_rects] = new_rect.potential_rect;
	    (*num_rects)++;
	    if (*num_rects >= *rect_storage) {
		(*rect_storage) *= 2;
		*editor_rect_list = (SDL_Rect*)realloc(*editor_rect_list, *rect_storage * sizeof(SDL_Rect));
	    }
	    want_new_rect = false;
	} else {
	    //logic here is a little trippy
	    if (*mouse_x < new_rect.original_rect.x) {
		if (*mouse_y < new_rect.original_rect.y) {
		    new_rect.potential_rect.x = *mouse_x;
		    new_rect.potential_rect.y = *mouse_y;
		    new_rect.potential_rect.w = new_rect.original_rect.x - *mouse_x;
		    new_rect.potential_rect.h = new_rect.original_rect.y - *mouse_y;
		} else {
		    new_rect.potential_rect.x = *mouse_x;
		    new_rect.potential_rect.y = new_rect.original_rect.y;
		    new_rect.potential_rect.w = new_rect.original_rect.x - *mouse_x;
		    new_rect.potential_rect.h = *mouse_y - new_rect.original_rect.y;
		}
	    } else {
		if (*mouse_y < new_rect.original_rect.y) {
		    new_rect.potential_rect.x = new_rect.original_rect.x;
		    new_rect.potential_rect.y = *mouse_y;
		    new_rect.potential_rect.w = *mouse_x - new_rect.original_rect.x;
		    new_rect.potential_rect.h = new_rect.original_rect.y - *mouse_y;

		} else {
		    new_rect.potential_rect.x = new_rect.original_rect.x;
		    new_rect.potential_rect.y = new_rect.original_rect.y;
		    new_rect.potential_rect.w = *mouse_x - new_rect.original_rect.x;
		    new_rect.potential_rect.h = *mouse_y - new_rect.original_rect.y;
		}

	    }

	}
    }

    if (want_save) {
	//make this prettier and in-app rather than console
	char file_name_buffer[50];
	get_string_stripped(file_name_buffer, 50);
	FILE *fp = fopen(file_name_buffer, "wb");
	if (fp == NULL) {
	    //error handling
	    printf("couldn't open file\n");
	}
	
	fwrite(*editor_rect_list, 1, *rect_storage * sizeof(SDL_Rect), fp);
	fclose(fp);

	/* possible to list directory contents?
	 * */
	HANDLE hFind = INVALID_HANDLE_VALUE;
	WIN32_FIND_DATA ffd;
	TCHAR szDir[MAX_PATH];
	hFind = FindFirstFile(szDir, &ffd);

	if (hFind == INVALID_HANDLE_VALUE) {
	    printf("yeah hFind didn't work\n");
	}
	
	
    }

    if (want_load) {
	//make this prettier and in-app rather than console
	char file_name_buffer[50];
	get_string_stripped(file_name_buffer, 50);
	FILE *fp = fopen(file_name_buffer, "rb");
	if (fp == NULL) {
	    //error handling
	    printf("couldn't open file\n");
	}
	//find size of file?
	int32 size_of_file;
	fseek(fp, 0, SEEK_END);
	size_of_file = ftell(fp);
	rewind(fp);
	*editor_rect_list = (SDL_Rect*)realloc(*editor_rect_list, size_of_file);
	fread(*editor_rect_list, size_of_file, 1, fp);
	fclose(fp);
    }
	    
    SDL_SetRenderDrawColor(sdl_renderer, 0xbf, 0x00, 0xaf, 0xff);
    SDL_RenderClear(sdl_renderer);
    
    SDL_SetRenderDrawColor(sdl_renderer, 0xff, 0x00, 0xff, 0xff);
    if (want_new_rect) {
	//blit the current status of new_rect
	SDL_RenderFillRect(sdl_renderer, &new_rect.potential_rect);
    }
    SDL_SetRenderDrawColor(sdl_renderer, 0xff, 0x00, 0x00, 0xff);
    for (uint32 i = 0; i < (*num_rects); i++) {
	SDL_RenderFillRect(sdl_renderer, &((*editor_rect_list)[i]));
    }

    SDL_RenderPresent(sdl_renderer);
}

void strip_newline(char *string)
{
    //string needs to have space alloced first
    uint32 count = 0;
    while (string[count] != '\0') {
	if (string[count] == '\n') {
	    string[count] = '\0';
	    return;
	}
	count++;
    }
}

void get_string_stripped(char *string, uint32 len)
{
    //string needs to have space alloced first
    string = fgets(string, len, stdin);
    strip_newline(string);
}
