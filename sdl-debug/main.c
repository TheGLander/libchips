#define SDL_MAIN_USE_CALLBACKS 1  /* use the callbacks instead of main() */
#include <stdlib.h>
#include <SDL3/SDL.h>
#include <SDL3/SDL_main.h>

#include "format-tws.h"
#include "formats.h"

#include "data/ccl/ccl_embeds.h"
#include "data/tws/tws_embeds.h"

/* We will use this renderer to draw into this window every frame. */
static SDL_Window *window = NULL;
static SDL_Renderer *renderer = NULL;
static SDL_Texture *texture = NULL;
static int texture_width = 0;
static int texture_height = 0;

static LevelSet* levelset = NULL;
static TWSSet* twsset = NULL;
static Level* level;
static TWSMetadata* solution;
static GameInputList input_list;
static uint32_t tick = 0;
static size_t level_num = 23 - 1;

#define TILE_SIZE 32
#define GRID_WIDTH 32
#define GRID_HEIGHT 32

#define TILES_PER_ROW 7
#define TILES_PER_COL 16

#define WINDOW_WIDTH GRID_WIDTH * TILE_SIZE
#define WINDOW_HEIGHT GRID_HEIGHT * TILE_SIZE

typedef struct LevelTwsPair {
    LevelSet* set;
    TWSSet* tws;
} LevelTwsPair;
DEFINE_RESULT(LevelTwsPair);

Result_LevelTwsPair loadsets(uint8_t const* levelset, size_t levelset_size, uint8_t const* tws, size_t tws_size) {
    LevelTwsPair pair = {};

    Result_LevelSetPtr res = parse_ccl(levelset, levelset_size);
    if (!res.success) {
        eprintf("%s\n", res.error);
        free(res.error);
        return res_err(LevelTwsPair, res.error);
    }
    pair.set = res.value;

    Result_TWSSetPtr tws_res = parse_tws(tws, tws_size);
    if (!tws_res.success) {
        eprintf("%s\n", tws_res.error);
        free(tws_res.error);
        return res_err(LevelTwsPair, tws_res.error);
    }
    pair.tws = tws_res.value;
    return res_val(LevelTwsPair, pair);
}

void make_level_solution() {
    tick = 0;
    if (level != NULL) {
        Level_free(level);
        level = NULL;
    }

    Ruleset const* logic;
    if (twsset->ruleset == Ruleset_MS) {
        logic = &ms_logic;
    } else {
        logic = &lynx_logic;
    }
    Result_LevelPtr level_res = LevelMetadata_make_level(&levelset->levels[level_num], logic);
    if (!level_res.success) {
        eprintf("%s\n", level_res.error);
        free(level_res.error);
        return;
    }
    level = level_res.value;
    SDL_SetWindowTitle(window, levelset->levels[level_num].title);

    solution = &twsset->solutions[level_num];
    Result_GameInputList input_list_res = TWSMetadata_prepare_inputs(solution);
    if (!input_list_res.success) {
        eprintf("%s\n", input_list_res.error);
        free(input_list_res.error);
        return;
    }
    input_list = input_list_res.value;

    Level_set_init_step_parity(level, solution->init_step_parity);
    Level_set_rff_dir(level, solution->rff_dir);
    Prng_init_seeded(Level_get_prng_ptr(level), solution->prng_seed);
}

/* This function runs once at startup. */
SDL_AppResult SDL_AppInit(void **appstate, int argc, char *argv[])
{
    SDL_Surface *surface = NULL;
    char *png_path = NULL;

    SDL_SetAppMetadata("Example Renderer Textures", "1.0", "com.example.renderer-textures");

    if (!SDL_Init(SDL_INIT_VIDEO)) {
        SDL_Log("Couldn't initialize SDL: %s", SDL_GetError());
        return SDL_APP_FAILURE;
    }

    if (!SDL_CreateWindowAndRenderer("examples/renderer/textures", WINDOW_WIDTH, WINDOW_HEIGHT, SDL_WINDOW_RESIZABLE, &window, &renderer)) {
        SDL_Log("Couldn't create window/renderer: %s", SDL_GetError());
        return SDL_APP_FAILURE;
    }
    SDL_SetRenderLogicalPresentation(renderer, WINDOW_WIDTH, WINDOW_HEIGHT, SDL_LOGICAL_PRESENTATION_LETTERBOX);

    /* Textures are pixel data that we upload to the video hardware for fast drawing. Lots of 2D
       engines refer to these as "sprites." We'll do a static texture (upload once, draw many
       times) with data from a png file. */

    /* SDL_Surface is pixel data the CPU can access. SDL_Texture is pixel data the GPU can access.
       Load a .png into a surface, move it to a texture from there. */
    SDL_asprintf(&png_path, "%stiles.png", SDL_GetBasePath());  /* allocate a string of the full file path */
    surface = SDL_LoadPNG(png_path);
    if (!surface) {
        SDL_Log("Couldn't load png: %s", SDL_GetError());
        return SDL_APP_FAILURE;
    }

    SDL_free(png_path);  /* done with this, the file is loaded. */

    texture_width = surface->w;
    texture_height = surface->h;

    texture = SDL_CreateTextureFromSurface(renderer, surface);
    if (!texture) {
        SDL_Log("Couldn't create static texture: %s", SDL_GetError());
        return SDL_APP_FAILURE;
    }

    SDL_DestroySurface(surface);  /* done with this, the texture has a copy of the pixels now. */

    Result_LevelTwsPair pair = loadsets(CID_PatchTests_ccl, sizeof(CID_PatchTests_ccl), CID_PatchTests_ms_tws, sizeof(CID_PatchTests_ms_tws));
    levelset = pair.value.set;
    twsset = pair.value.tws;

    make_level_solution();

    return SDL_APP_CONTINUE;  /* carry on with the program! */
}

void rewind_solution(size_t num_ticks) {
    size_t temp_tick = tick;
    make_level_solution();
    if (temp_tick <= num_ticks)
        return;
    tick = temp_tick - num_ticks;
    size_t i = 0;
    while (i < tick) {
        Level_set_game_input(level, GameInputList_get_input(&input_list, i));
        Level_tick(level);
        i += 1;
    }
}

void advance_solution() {
    if (tick >= input_list.count + 1) {
        return;
    }
    // while ((level->game_input = solution->input_list.inputs[tick]) == DIRECTION_NIL) {
    //     Level_tick(level);
    //     tick++;
    // }
    Level_set_game_input(level, GameInputList_get_input(&input_list, tick));
    Level_tick(level);
    tick += 1;
}

static SDL_AppResult handle_key_event(SDL_Scancode key_code, SDL_Keymod keymod) {
    switch (key_code) {
        case SDL_SCANCODE_RETURN:
            advance_solution();
            if (!(keymod & SDL_KMOD_CTRL)) {
                advance_solution();
            }
            if (keymod & SDL_KMOD_SHIFT) {
                advance_solution();
                advance_solution();
            }
            break;
        case SDL_SCANCODE_BACKSPACE:
            size_t ticks = 1;
            if (!(keymod & SDL_KMOD_CTRL)) {
                ticks = 2;
            }
            if (keymod & SDL_KMOD_SHIFT) {
                ticks = 4;
            }
            rewind_solution(ticks);
            break;
        case SDL_SCANCODE_R:
            make_level_solution();
            break;
        case SDL_SCANCODE_P:
            if (level_num == 0)
                break;
            level_num--;
            make_level_solution();
            break;
        case SDL_SCANCODE_N:
            if (level_num >= levelset->levels_n)
                break;
            level_num++;
            make_level_solution();
            break;
        case SDL_SCANCODE_V:
            while (tick < input_list.count) {
                advance_solution();
            }
        default:
            break;
    }
    return SDL_APP_CONTINUE;
}

/* This function runs when a new event (mouse input, keypresses, etc) occurs. */
SDL_AppResult SDL_AppEvent(void *appstate, SDL_Event *event) {
    if (event->type == SDL_EVENT_QUIT) {
        return SDL_APP_SUCCESS;  /* end the program, reporting success to the OS. */
    } else if (event->type == SDL_EVENT_KEY_DOWN) {
        return handle_key_event(event->key.scancode, event->key.mod);
    }
    return SDL_APP_CONTINUE;  /* carry on with the program! */
}

#define	_NORTH	+ 0
#define	_WEST	+ 1
#define	_SOUTH	+ 2
#define	_EAST	+ 3

typedef	struct tileidinfo {
    int		id;		/* the tile ID */
    signed char	xopaque;	/* the coordinates of the opaque image */
    signed char	yopaque;	/*   (expressed in tiles, not pixels) */
    signed char	xtransp;	/* coordinates of the transparent image */
    signed char	ytransp;	/*   (also expressed in tiles) */
    int		shape;		/* enum values for the free-form bitmap */
} tileidinfo;

/* The list of tile images.
 */
enum {
    TILEIMG_IMPLICIT = 0,	/* tile is not in the bitmap */
    TILEIMG_SINGLEOPAQUE,	/* a single opaque image */
    TILEIMG_OPAQUECELS,		/* one or more opaque images */
    TILEIMG_TRANSPCELS,		/* one or more transparent images */
    TILEIMG_CREATURE,		/* one of the creature formats */
    TILEIMG_ANIMATION		/* twelve transparent images */
};
static tileidinfo const tileidmap[] = {
    { Empty,			 0,  0, -1, -1, TILEIMG_SINGLEOPAQUE },
    { Slide_North,		 1,  2, -1, -1, TILEIMG_OPAQUECELS },
    { Slide_West,		 1,  4, -1, -1, TILEIMG_OPAQUECELS },
    { Slide_South,		 0, 13, -1, -1, TILEIMG_OPAQUECELS },
    { Slide_East,		 1,  3, -1, -1, TILEIMG_OPAQUECELS },
    { Slide_Random,		 3,  2, -1, -1, TILEIMG_OPAQUECELS },
    { Ice,			 0, 12, -1, -1, TILEIMG_OPAQUECELS },
    { IceWall_Northwest,	 1, 12, -1, -1, TILEIMG_OPAQUECELS },
    { IceWall_Northeast,	 1, 13, -1, -1, TILEIMG_OPAQUECELS },
    { IceWall_Southwest,	 1, 11, -1, -1, TILEIMG_OPAQUECELS },
    { IceWall_Southeast,	 1, 10, -1, -1, TILEIMG_OPAQUECELS },
    { Gravel,			 2, 13, -1, -1, TILEIMG_OPAQUECELS },
    { Dirt,			 0, 11, -1, -1, TILEIMG_OPAQUECELS },
    { Water,			 0,  3, -1, -1, TILEIMG_OPAQUECELS },
    { Fire,			 0,  4, -1, -1, TILEIMG_OPAQUECELS },
    { Bomb,			 2, 10, -1, -1, TILEIMG_OPAQUECELS },
    { Beartrap,			 2, 11, -1, -1, TILEIMG_OPAQUECELS },
    { Burglar,			 2,  1, -1, -1, TILEIMG_OPAQUECELS },
    { HintButton,		 2, 15, -1, -1, TILEIMG_OPAQUECELS },
    { Button_Blue,		 2,  8, -1, -1, TILEIMG_OPAQUECELS },
    { Button_Green,		 2,  3, -1, -1, TILEIMG_OPAQUECELS },
    { Button_Red,		 2,  4, -1, -1, TILEIMG_OPAQUECELS },
    { Button_Brown,		 2,  7, -1, -1, TILEIMG_OPAQUECELS },
    { Teleport,			 2,  9, -1, -1, TILEIMG_OPAQUECELS },
    { Wall,			 0,  1, -1, -1, TILEIMG_OPAQUECELS },
    { Wall_North,		 0,  6, -1, -1, TILEIMG_OPAQUECELS },
    { Wall_West,		 0,  7, -1, -1, TILEIMG_OPAQUECELS },
    { Wall_South,		 0,  8, -1, -1, TILEIMG_OPAQUECELS },
    { Wall_East,		 0,  9, -1, -1, TILEIMG_OPAQUECELS },
    { Wall_Southeast,		 3,  0, -1, -1, TILEIMG_OPAQUECELS },
    { HiddenWall_Perm,		 0,  5, -1, -1, TILEIMG_IMPLICIT },
    { HiddenWall_Temp,		 2, 12, -1, -1, TILEIMG_IMPLICIT },
    { BlueWall_Real,		 1, 15, -1, -1, TILEIMG_OPAQUECELS },
    { BlueWall_Fake,		 1, 14, -1, -1, TILEIMG_IMPLICIT },
    { SwitchWall_Open,		 2,  6, -1, -1, TILEIMG_OPAQUECELS },
    { SwitchWall_Closed,	 2,  5, -1, -1, TILEIMG_OPAQUECELS },
    { PopupWall,		 2, 14, -1, -1, TILEIMG_OPAQUECELS },
    { CloneMachine,		 3,  1, -1, -1, TILEIMG_OPAQUECELS },
    { Door_Red,			 1,  7, -1, -1, TILEIMG_OPAQUECELS },
    { Door_Blue,		 1,  6, -1, -1, TILEIMG_OPAQUECELS },
    { Door_Yellow,		 1,  9, -1, -1, TILEIMG_OPAQUECELS },
    { Door_Green,		 1,  8, -1, -1, TILEIMG_OPAQUECELS },
    { Socket,			 2,  2, -1, -1, TILEIMG_OPAQUECELS },
    { Exit,			 1,  5, -1, -1, TILEIMG_OPAQUECELS },
    { ICChip,			 0,  2, -1, -1, TILEIMG_OPAQUECELS },
    { Key_Red,			 6,  5,  9,  5, TILEIMG_TRANSPCELS },
    { Key_Blue,			 6,  4,  9,  4, TILEIMG_TRANSPCELS },
    { Key_Yellow,		 6,  7,  9,  7, TILEIMG_TRANSPCELS },
    { Key_Green,		 6,  6,  9,  6, TILEIMG_TRANSPCELS },
    { Boots_Ice,		 6, 10,  9, 10, TILEIMG_TRANSPCELS },
    { Boots_Slide,		 6, 11,  9, 11, TILEIMG_TRANSPCELS },
    { Boots_Fire,		 6,  9,  9,  9, TILEIMG_TRANSPCELS },
    { Boots_Water,		 6,  8,  9,  8, TILEIMG_TRANSPCELS },
    { Block_Static,		 0, 10, -1, -1, TILEIMG_IMPLICIT },
    { Overlay_Buffer,		 2,  0, -1, -1, TILEIMG_IMPLICIT },
    { Exit_Extra_1,		 3, 10, -1, -1, TILEIMG_SINGLEOPAQUE },
    { Exit_Extra_2,		 3, 11, -1, -1, TILEIMG_SINGLEOPAQUE },
    { Burned_Chip,		 3,  4, -1, -1, TILEIMG_SINGLEOPAQUE },
    { Bombed_Chip,		 3,  5, -1, -1, TILEIMG_SINGLEOPAQUE },
    { Exited_Chip,		 3,  9, -1, -1, TILEIMG_SINGLEOPAQUE },
    { Drowned_Chip,		 3,  3, -1, -1, TILEIMG_SINGLEOPAQUE },
    { Swimming_Chip _NORTH,	 3, 12, -1, -1, TILEIMG_SINGLEOPAQUE },
    { Swimming_Chip _WEST,	 3, 13, -1, -1, TILEIMG_SINGLEOPAQUE },
    { Swimming_Chip _SOUTH,	 3, 14, -1, -1, TILEIMG_SINGLEOPAQUE },
    { Swimming_Chip _EAST,	 3, 15, -1, -1, TILEIMG_SINGLEOPAQUE },
    { Chip _NORTH,		 6, 12,  9, 12, TILEIMG_CREATURE },
    { Chip _WEST,		 6, 13,  9, 13, TILEIMG_IMPLICIT },
    { Chip _SOUTH,		 6, 14,  9, 14, TILEIMG_IMPLICIT },
    { Chip _EAST,		 6, 15,  9, 15, TILEIMG_IMPLICIT },
    { Pushing_Chip _NORTH,	 6, 12,  9, 12, TILEIMG_CREATURE },
    { Pushing_Chip _WEST,	 6, 13,  9, 13, TILEIMG_IMPLICIT },
    { Pushing_Chip _SOUTH,	 6, 14,  9, 14, TILEIMG_IMPLICIT },
    { Pushing_Chip _EAST,	 6, 15,  9, 15, TILEIMG_IMPLICIT },
    { Block _NORTH,		 0, 14, -1, -1, TILEIMG_CREATURE },
    { Block _WEST,		 0, 15, -1, -1, TILEIMG_IMPLICIT },
    { Block _SOUTH,		 1,  0, -1, -1, TILEIMG_IMPLICIT },
    { Block _EAST,		 1,  1, -1, -1, TILEIMG_IMPLICIT },
    { Tank _NORTH,		 4, 12,  7, 12, TILEIMG_CREATURE },
    { Tank _WEST,		 4, 13,  7, 13, TILEIMG_IMPLICIT },
    { Tank _SOUTH,		 4, 14,  7, 14, TILEIMG_IMPLICIT },
    { Tank _EAST,		 4, 15,  7, 15, TILEIMG_IMPLICIT },
    { Ball _NORTH,		 4,  8,  7,  8, TILEIMG_CREATURE },
    { Ball _WEST,		 4,  9,  7,  9, TILEIMG_IMPLICIT },
    { Ball _SOUTH,		 4, 10,  7, 10, TILEIMG_IMPLICIT },
    { Ball _EAST,		 4, 11,  7, 11, TILEIMG_IMPLICIT },
    { Glider _NORTH,		 5,  0,  8,  0, TILEIMG_CREATURE },
    { Glider _WEST,		 5,  1,  8,  1, TILEIMG_IMPLICIT },
    { Glider _SOUTH,		 5,  2,  8,  2, TILEIMG_IMPLICIT },
    { Glider _EAST,		 5,  3,  8,  3, TILEIMG_IMPLICIT },
    { Fireball _NORTH,		 4,  4,  7,  4, TILEIMG_CREATURE },
    { Fireball _WEST,		 4,  5,  7,  5, TILEIMG_IMPLICIT },
    { Fireball _SOUTH,		 4,  6,  7,  6, TILEIMG_IMPLICIT },
    { Fireball _EAST,		 4,  7,  7,  7, TILEIMG_IMPLICIT },
    { Bug _NORTH,		 4,  0,  7,  0, TILEIMG_CREATURE },
    { Bug _WEST,		 4,  1,  7,  1, TILEIMG_IMPLICIT },
    { Bug _SOUTH,		 4,  2,  7,  2, TILEIMG_IMPLICIT },
    { Bug _EAST,		 4,  3,  7,  3, TILEIMG_IMPLICIT },
    { Paramecium _NORTH,	 6,  0,  9,  0, TILEIMG_CREATURE },
    { Paramecium _WEST,		 6,  1,  9,  1, TILEIMG_IMPLICIT },
    { Paramecium _SOUTH,	 6,  2,  9,  2, TILEIMG_IMPLICIT },
    { Paramecium _EAST,		 6,  3,  9,  3, TILEIMG_IMPLICIT },
    { Teeth _NORTH,		 5,  4,  8,  4, TILEIMG_CREATURE },
    { Teeth _WEST,		 5,  5,  8,  5, TILEIMG_IMPLICIT },
    { Teeth _SOUTH,		 5,  6,  8,  6, TILEIMG_IMPLICIT },
    { Teeth _EAST,		 5,  7,  8,  7, TILEIMG_IMPLICIT },
    { Blob _NORTH,		 5, 12,  8, 12, TILEIMG_CREATURE },
    { Blob _WEST,		 5, 13,  8, 13, TILEIMG_IMPLICIT },
    { Blob _SOUTH,		 5, 14,  8, 14, TILEIMG_IMPLICIT },
    { Blob _EAST,		 5, 15,  8, 15, TILEIMG_IMPLICIT },
    { Walker _NORTH,		 5,  8,  8,  8, TILEIMG_CREATURE },
    { Walker _WEST,		 5,  9,  8,  9, TILEIMG_IMPLICIT },
    { Walker _SOUTH,		 5, 10,  8, 10, TILEIMG_IMPLICIT },
    { Walker _EAST,		 5, 11,  8, 11, TILEIMG_IMPLICIT },
    { Water_Splash,		 3,  3, -1, -1, TILEIMG_ANIMATION },
    { Bomb_Explosion,		 3,  6, -1, -1, TILEIMG_ANIMATION },
    { Entity_Explosion,		 3,  7, -1, -1, TILEIMG_ANIMATION }
};

static tileidinfo get_tileidinfo(TileID id) {
    for (size_t i = 0; i < sizeof(tileidmap) / sizeof(tileidmap[0]); i++) {
        if (tileidmap[i].id == id) {
            return tileidmap[i];
        }
    }
    return tileidmap[0];
}

/* This function runs once per frame, and is the heart of the program. */
SDL_AppResult SDL_AppIterate(void *appstate)
{
    SDL_FRect src_rect;
    SDL_FRect dst_rect;
    const Uint64 now = SDL_GetTicks();

    /* we'll have some textures move around over a few seconds. */
    // const float direction = ((now % 2000) >= 1000) ? 1.0f : -1.0f;
    // const float scale = ((float) (((int) (now % 1000)) - 500) / 500.0f) * direction;

    /* as you can see from this, rendering draws over whatever was drawn before it. */
    SDL_SetRenderDrawColor(renderer, 0, 0, 0, SDL_ALPHA_OPAQUE);  /* black, full alpha */
    SDL_RenderClear(renderer);  /* start with a blank canvas. */

    src_rect.w = TILE_SIZE;
    src_rect.h = TILE_SIZE;
    dst_rect.w = TILE_SIZE;
    dst_rect.h = TILE_SIZE;
    for (uint32_t y = 0; y < GRID_HEIGHT; y++) {
        for (uint32_t x = 0; x < GRID_WIDTH; x++) {
            dst_rect.x = (x * TILE_SIZE);
            dst_rect.y = (y * TILE_SIZE);

            uint32_t pos = x + y * GRID_WIDTH;
            MapCell cell = level->map[pos];
            tileidinfo top_info = get_tileidinfo(cell.top.id);
            tileidinfo bot_info = get_tileidinfo(cell.bottom.id);

            src_rect.x = bot_info.xopaque * TILE_SIZE;
            src_rect.y = bot_info.yopaque * TILE_SIZE;
            SDL_RenderTexture(renderer, texture, &src_rect, &dst_rect);

            src_rect.x = top_info.xopaque * TILE_SIZE;
            src_rect.y = top_info.yopaque * TILE_SIZE;
            SDL_RenderTexture(renderer, texture, &src_rect, &dst_rect);
        }
    }
    if (twsset->ruleset == Ruleset_Lynx) {
        Actor* actors = Level_get_actors_ptr(level);
        for (Actor* actor = actors; actor->id != Nothing; actor += 1) {
            if (Actor_get_hidden(actor) && Actor_get_id(actor) != Chip)
                continue;

            TileID id = Actor_get_id(actor);
            Direction dir = Actor_get_direction(actor);
            if (TileID_is_animation(id))
                dir = DIRECTION_NIL;
            float x_offset = 0;
            float y_offset = 0;
            if (dir == DIRECTION_WEST || dir == DIRECTION_EAST) {
                x_offset = ((Actor_get_animation_frame(actor)) / 4.f) * TILE_SIZE * (dir == DIRECTION_WEST ? 1.f : -1.f);
            } else if (dir == DIRECTION_SOUTH || dir == DIRECTION_NORTH) {
                y_offset = ((Actor_get_animation_frame(actor)) / 4.f) * TILE_SIZE * (dir == DIRECTION_NORTH ? 1.f : -1.f);
            }

            dst_rect.x = ((Actor_get_position(actor) % GRID_WIDTH) * TILE_SIZE) + x_offset;
            dst_rect.y = ((Actor_get_position(actor) / GRID_WIDTH) * TILE_SIZE) + y_offset;

            if (dir == DIRECTION_WEST)
                id += 1;
            else if (dir == DIRECTION_SOUTH)
                id += 2;
            else if (dir == DIRECTION_EAST)
                id += 3;
            tileidinfo cr_info = get_tileidinfo(id);
            src_rect.x = cr_info.xopaque * TILE_SIZE;
            src_rect.y = cr_info.yopaque * TILE_SIZE;
            SDL_RenderTexture(renderer, texture, &src_rect, &dst_rect);
        }
    }

    SDL_RenderPresent(renderer);  /* put it all on the screen! */

    return SDL_APP_CONTINUE;  /* carry on with the program! */
}

/* This function runs once at shutdown. */
void SDL_AppQuit(void *appstate, SDL_AppResult result)
{
    SDL_DestroyTexture(texture);
    /* SDL will clean up the window/renderer for us. */
}