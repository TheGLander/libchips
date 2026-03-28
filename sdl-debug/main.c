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
static size_t level_num = 7;

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
    if (TWSSet_get_ruleset(twsset) == RULESET_MS) {
        logic = &ms_logic;
    } else {
        logic = &lynx_logic;
    }
    Result_LevelPtr level_res = LevelMetadata_make_level(LevelSet_get_level(levelset, level_num - 1), logic);
    if (!level_res.success) {
        eprintf("%s\n", level_res.error);
        free(level_res.error);
        return;
    }
    level = level_res.value;
    SDL_SetWindowTitle(window, LevelMetadata_get_title(LevelSet_get_level(levelset, level_num - 1)));

    solution = TWSSet_get_solution_by_level_num(twsset, level_num);
    Result_GameInputList input_list_res = TWSMetadata_prepare_inputs(solution);
    if (!input_list_res.success) {
        eprintf("%s\n", input_list_res.error);
        free(input_list_res.error);
        return;
    }
    input_list = input_list_res.value;

    Level_set_init_step_parity(level, TWSMetadata_get_init_step_parity(solution));
    Level_set_rff_dir(level, TWSMetadata_get_rff_dir(solution));
    Prng_init_seeded(Level_get_prng_ptr(level), TWSMetadata_get_prng_seed(solution));
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

    Result_LevelTwsPair pair = loadsets(CCLP1_ccl, sizeof(CCLP1_ccl), public_CCLP1_tws, sizeof(public_CCLP1_tws));
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
            if (level_num >= LevelSet_get_levels_n(levelset))
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
    TileID		id;		/* the tile ID */
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
    { FLOOR,			 0,  0, -1, -1, TILEIMG_SINGLEOPAQUE },
    { FORCE_FLOOR_NORTH,		 1,  2, -1, -1, TILEIMG_OPAQUECELS },
    { FORCE_FLOOR_WEST,		 1,  4, -1, -1, TILEIMG_OPAQUECELS },
    { FORCE_FLOOR_SOUTH,		 0, 13, -1, -1, TILEIMG_OPAQUECELS },
    { FORCE_FLOOR_EAST,		 1,  3, -1, -1, TILEIMG_OPAQUECELS },
    { FORCE_FLOOR_RANDOM,		 3,  2, -1, -1, TILEIMG_OPAQUECELS },
    { ICE,			 0, 12, -1, -1, TILEIMG_OPAQUECELS },
    { ICE_CORNER_NORTH_WEST,	 1, 12, -1, -1, TILEIMG_OPAQUECELS },
    { ICE_CORNER_NORTH_EAST,	 1, 13, -1, -1, TILEIMG_OPAQUECELS },
    { ICE_CORNER_SOUTH_WEST,	 1, 11, -1, -1, TILEIMG_OPAQUECELS },
    { ICE_CORNER_SOUTH_EAST,	 1, 10, -1, -1, TILEIMG_OPAQUECELS },
    { GRAVEL,			 2, 13, -1, -1, TILEIMG_OPAQUECELS },
    { DIRT,			 0, 11, -1, -1, TILEIMG_OPAQUECELS },
    { WATER,			 0,  3, -1, -1, TILEIMG_OPAQUECELS },
    { FIRE,			 0,  4, -1, -1, TILEIMG_OPAQUECELS },
    { BOMB,			 2, 10, -1, -1, TILEIMG_OPAQUECELS },
    { TRAP,			 2, 11, -1, -1, TILEIMG_OPAQUECELS },
    { THIEF,			 2,  1, -1, -1, TILEIMG_OPAQUECELS },
    { HINT,		 2, 15, -1, -1, TILEIMG_OPAQUECELS },
    { BUTTON_TANK,		 2,  8, -1, -1, TILEIMG_OPAQUECELS },
    { BUTTON_TOGGLE,		 2,  3, -1, -1, TILEIMG_OPAQUECELS },
    { BUTTON_CLONE,		 2,  4, -1, -1, TILEIMG_OPAQUECELS },
    { BUTTON_TRAP,		 2,  7, -1, -1, TILEIMG_OPAQUECELS },
    { TELEPORT,			 2,  9, -1, -1, TILEIMG_OPAQUECELS },
    { WALL,			 0,  1, -1, -1, TILEIMG_OPAQUECELS },
    { THIN_WALL_NORTH,		 0,  6, -1, -1, TILEIMG_OPAQUECELS },
    { THIN_WALL_WEST,		 0,  7, -1, -1, TILEIMG_OPAQUECELS },
    { THIN_WALL_SOUTH,		 0,  8, -1, -1, TILEIMG_OPAQUECELS },
    { THIN_WALL_EAST,		 0,  9, -1, -1, TILEIMG_OPAQUECELS },
    { THIN_WALL_SOUTH_EAST,		 3,  0, -1, -1, TILEIMG_OPAQUECELS },
    { INVISIBLE_WALL,		 0,  5, -1, -1, TILEIMG_IMPLICIT },
    { HIDDEN_WALL,		 2, 12, -1, -1, TILEIMG_IMPLICIT },
    { BLUE_WALL_REAL,		 1, 15, -1, -1, TILEIMG_OPAQUECELS },
    { BLUE_WALL_FAKE,		 1, 14, -1, -1, TILEIMG_IMPLICIT },
    { TOGGLE_DOOR_OPEN,		 2,  6, -1, -1, TILEIMG_OPAQUECELS },
    { TOGGLE_DOOR_CLOSED,	 2,  5, -1, -1, TILEIMG_OPAQUECELS },
    { POPUP_WALL,		 2, 14, -1, -1, TILEIMG_OPAQUECELS },
    { CLONE_MACHINE,		 3,  1, -1, -1, TILEIMG_OPAQUECELS },
    { DOOR_RED,			 1,  7, -1, -1, TILEIMG_OPAQUECELS },
    { DOOR_BLUE,		 1,  6, -1, -1, TILEIMG_OPAQUECELS },
    { DOOR_YELLOW,		 1,  9, -1, -1, TILEIMG_OPAQUECELS },
    { DOOR_GREEN,		 1,  8, -1, -1, TILEIMG_OPAQUECELS },
    { SOCKET,			 2,  2, -1, -1, TILEIMG_OPAQUECELS },
    { EXIT,			 1,  5, -1, -1, TILEIMG_OPAQUECELS },
    { IC_CHIP,			 0,  2, -1, -1, TILEIMG_OPAQUECELS },
    { KEY_RED,			 6,  5,  9,  5, TILEIMG_TRANSPCELS },
    { KEY_BLUE,			 6,  4,  9,  4, TILEIMG_TRANSPCELS },
    { KEY_YELLOW,		 6,  7,  9,  7, TILEIMG_TRANSPCELS },
    { KEY_GREEN,		 6,  6,  9,  6, TILEIMG_TRANSPCELS },
    { BOOTS_ICE,		 6, 10,  9, 10, TILEIMG_TRANSPCELS },
    { BOOTS_FORCE_FLOOR,		 6, 11,  9, 11, TILEIMG_TRANSPCELS },
    { BOOTS_FIRE,		 6,  9,  9,  9, TILEIMG_TRANSPCELS },
    { BOOTS_WATER,		 6,  8,  9,  8, TILEIMG_TRANSPCELS },
    { BLOCK_STATIC,		 0, 10, -1, -1, TILEIMG_IMPLICIT },
    { OVERLAY_BUFFER,		 2,  0, -1, -1, TILEIMG_IMPLICIT },
    { EXIT_ANIM_1,		 3, 10, -1, -1, TILEIMG_SINGLEOPAQUE },
    { EXIT_ANIM_2,		 3, 11, -1, -1, TILEIMG_SINGLEOPAQUE },
    { BURNED_CHIP,		 3,  4, -1, -1, TILEIMG_SINGLEOPAQUE },
    { BOMBED_CHIP,		 3,  5, -1, -1, TILEIMG_SINGLEOPAQUE },
    { EXITED_CHIP,		 3,  9, -1, -1, TILEIMG_SINGLEOPAQUE },
    { DROWNED_CHIP,		 3,  3, -1, -1, TILEIMG_SINGLEOPAQUE },
    { SWIMMING_CHIP _NORTH,	 3, 12, -1, -1, TILEIMG_SINGLEOPAQUE },
    { SWIMMING_CHIP _WEST,	 3, 13, -1, -1, TILEIMG_SINGLEOPAQUE },
    { SWIMMING_CHIP _SOUTH,	 3, 14, -1, -1, TILEIMG_SINGLEOPAQUE },
    { SWIMMING_CHIP _EAST,	 3, 15, -1, -1, TILEIMG_SINGLEOPAQUE },
    { CHIP _NORTH,		 6, 12,  9, 12, TILEIMG_CREATURE },
    { CHIP _WEST,		 6, 13,  9, 13, TILEIMG_IMPLICIT },
    { CHIP _SOUTH,		 6, 14,  9, 14, TILEIMG_IMPLICIT },
    { CHIP _EAST,		 6, 15,  9, 15, TILEIMG_IMPLICIT },
    { PUSHING_CHIP _NORTH,	 6, 12,  9, 12, TILEIMG_CREATURE },
    { PUSHING_CHIP _WEST,	 6, 13,  9, 13, TILEIMG_IMPLICIT },
    { PUSHING_CHIP _SOUTH,	 6, 14,  9, 14, TILEIMG_IMPLICIT },
    { PUSHING_CHIP _EAST,	 6, 15,  9, 15, TILEIMG_IMPLICIT },
    { BLOCK _NORTH,		 0, 14, -1, -1, TILEIMG_CREATURE },
    { BLOCK _WEST,		 0, 15, -1, -1, TILEIMG_IMPLICIT },
    { BLOCK _SOUTH,		 1,  0, -1, -1, TILEIMG_IMPLICIT },
    { BLOCK _EAST,		 1,  1, -1, -1, TILEIMG_IMPLICIT },
    { TANK _NORTH,		 4, 12,  7, 12, TILEIMG_CREATURE },
    { TANK _WEST,		 4, 13,  7, 13, TILEIMG_IMPLICIT },
    { TANK _SOUTH,		 4, 14,  7, 14, TILEIMG_IMPLICIT },
    { TANK _EAST,		 4, 15,  7, 15, TILEIMG_IMPLICIT },
    { BALL _NORTH,		 4,  8,  7,  8, TILEIMG_CREATURE },
    { BALL _WEST,		 4,  9,  7,  9, TILEIMG_IMPLICIT },
    { BALL _SOUTH,		 4, 10,  7, 10, TILEIMG_IMPLICIT },
    { BALL _EAST,		 4, 11,  7, 11, TILEIMG_IMPLICIT },
    { GLIDER _NORTH,		 5,  0,  8,  0, TILEIMG_CREATURE },
    { GLIDER _WEST,		 5,  1,  8,  1, TILEIMG_IMPLICIT },
    { GLIDER _SOUTH,		 5,  2,  8,  2, TILEIMG_IMPLICIT },
    { GLIDER _EAST,		 5,  3,  8,  3, TILEIMG_IMPLICIT },
    { FIREBALL _NORTH,		 4,  4,  7,  4, TILEIMG_CREATURE },
    { FIREBALL _WEST,		 4,  5,  7,  5, TILEIMG_IMPLICIT },
    { FIREBALL _SOUTH,		 4,  6,  7,  6, TILEIMG_IMPLICIT },
    { FIREBALL _EAST,		 4,  7,  7,  7, TILEIMG_IMPLICIT },
    { BUG _NORTH,		 4,  0,  7,  0, TILEIMG_CREATURE },
    { BUG _WEST,		 4,  1,  7,  1, TILEIMG_IMPLICIT },
    { BUG _SOUTH,		 4,  2,  7,  2, TILEIMG_IMPLICIT },
    { BUG _EAST,		 4,  3,  7,  3, TILEIMG_IMPLICIT },
    { PARAMECIUM _NORTH,	 6,  0,  9,  0, TILEIMG_CREATURE },
    { PARAMECIUM _WEST,		 6,  1,  9,  1, TILEIMG_IMPLICIT },
    { PARAMECIUM _SOUTH,	 6,  2,  9,  2, TILEIMG_IMPLICIT },
    { PARAMECIUM _EAST,		 6,  3,  9,  3, TILEIMG_IMPLICIT },
    { TEETH _NORTH,		 5,  4,  8,  4, TILEIMG_CREATURE },
    { TEETH _WEST,		 5,  5,  8,  5, TILEIMG_IMPLICIT },
    { TEETH _SOUTH,		 5,  6,  8,  6, TILEIMG_IMPLICIT },
    { TEETH _EAST,		 5,  7,  8,  7, TILEIMG_IMPLICIT },
    { BLOB _NORTH,		 5, 12,  8, 12, TILEIMG_CREATURE },
    { BLOB _WEST,		 5, 13,  8, 13, TILEIMG_IMPLICIT },
    { BLOB _SOUTH,		 5, 14,  8, 14, TILEIMG_IMPLICIT },
    { BLOB _EAST,		 5, 15,  8, 15, TILEIMG_IMPLICIT },
    { WALKER _NORTH,		 5,  8,  8,  8, TILEIMG_CREATURE },
    { WALKER _WEST,		 5,  9,  8,  9, TILEIMG_IMPLICIT },
    { WALKER _SOUTH,		 5, 10,  8, 10, TILEIMG_IMPLICIT },
    { WALKER _EAST,		 5, 11,  8, 11, TILEIMG_IMPLICIT },
    { ANIM_WATER,		 3,  3, -1, -1, TILEIMG_ANIMATION },
    { ANIM_BOMB,		 3,  6, -1, -1, TILEIMG_ANIMATION },
    { ANIM_ENTITY,		 3,  7, -1, -1, TILEIMG_ANIMATION }
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
            tileidinfo top_info = get_tileidinfo(Level_get_top_terrain(level, pos));
            tileidinfo bot_info = get_tileidinfo(Level_get_bottom_terrain(level, pos));

            src_rect.x = bot_info.xopaque * TILE_SIZE;
            src_rect.y = bot_info.yopaque * TILE_SIZE;
            SDL_RenderTexture(renderer, texture, &src_rect, &dst_rect);

            src_rect.x = top_info.xopaque * TILE_SIZE;
            src_rect.y = top_info.yopaque * TILE_SIZE;
            SDL_RenderTexture(renderer, texture, &src_rect, &dst_rect);
        }
    }
    if (Level_get_ruleset(level)->id == RULESET_LYNX) {
        Actor* actors = Level_get_actors_ptr(level);
        for (Actor* actor = actors; Actor_get_id(actor) != NOTHING; actor += 1) {
            if (Actor_get_hidden(actor) && Actor_get_id(actor) != CHIP)
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