#include "maps/map3.h"
#include "data/assets.h"

// #pragma bss-name(push, "ZEROPAGE")

// SIZE: 256x240 Pixels -> 32x30 Tiles -> 16x15 Palette Blocks -> 960

#define WIDTH_IN_TILES 32
#define HEIGHT_IN_TILES 30

#define TEXT_HEIGHT 0
#define VALUE_HEIGHT 1

// Game variables location
#define POPULATION_START 4
#define POPULATION_LENGTH 10
#define NET_WORTH_START 18
#define NET_WORTH_LENGTH 10

// Building Choice Menu location
#define CHOICE_X_TILE 20
#define CHOICE_START 8
#define CHOICE_END 20
#define CHOICE_WIDTH 8

#define PIXELS_TO_INDEX(x, y) ((x) >> 3) + ((y) >> 3) * WIDTH_IN_TILES
#define TILES_TO_INDEX(x, y) (x) + (y) * WIDTH_IN_TILES
#define IS_ON_WATER(val) ((val >= 0xc0 && val <= 0xCB) || (val != 0xd4 && val >= 0xd0 && val <= 0xd5) || (val < 0x80))

ctype* POPULATION_TEXT = STR("POPULATION");
ctype* NET_WORTH_TEXT = STR("NET WORTH$");

ulong net_worth = 100;
ulong population = 100;

ulong tick = 0;

// Fram tick counter for continous addition of population
type population_buffer = 0;
type tmp_data[] = {0, 0, 0, 0, 0, 0, 0, 0};

type tmp_3 = NULL;
type tmp_4 = NULL;
type tmp_5 = NULL;
type *tmp_arr = {0};

// Map pointer X of pixels
type pointer_x = 16;
// Map pointer Y of pixels
type pointer_y = 32;

type pad1 = 0;
type pad_new = 0;

#define WAS_IN_FIRE(building) (building.kind & 0b10000)

typedef struct Building {
	// Level and type of building (...xx - level, ...xx00 - type), ...x0000 - was in fire
	type kind;
	// Building meta data: income, population, etc.
	type b_data;
	// Index on map (in 8x8 Tiles)
	uint location;
} Building;

List(Building) buildings;

// Idx of building level
type choice_pointer_x_id = 0;
// Idx of building type
type choice_pointer_y_id = 0;
type choice_selected = 0;
// Counts in 8x8 blocks since CHOICE_START
type choice_idx = CHOICE_START;
type is_choice_closed = 1;

void render_pointer(void) {
    oam_meta_spr(pointer_x, pointer_y, pointer);
	if(choice_idx == CHOICE_END) oam_meta_spr(CHOICE_X_TILE * 8 + choice_pointer_x_id * 16, CHOICE_START * 8 + choice_pointer_y_id * 16 - 1, choice_pointer);
}

type palette_tmp[] = {0};

#define IS_LIVING_BUILDING(idx) (((idx & 0b1100) >> 2) == 1 || ((idx & 0b1100) >> 2) == 2)
#define IS_INDUSTRY_BUILDING(idx) (((idx & 0b1100) >> 2) == 3)
#define IS_COMMERCIAL_BUILDING(idx) (((idx & 0b1100) >> 2) == 0 || ((idx & 0b1100) >> 2) == 4)
#define GRASS_TILE 0xD4
#define CAN_BUILD map[CURSOR_TO_MAP_INDEX] = GRASS_TILE
#define UPDATE_PALETTE_BLOCK(old, to_insert, x, y) old & ~(3 << (2 * ((x / 16 % 2) + 2 * (y / 16 % 2)))) | (to_insert << (2 * ((x / 16 % 2) + 2 * (y / 16 % 2))))
#define CURSOR_TO_MAP_INDEX (pointer_x >> 3) + (pointer_y >> 3) * WIDTH_IN_TILES
// https://www.nesdev.org/wiki/PPU_attribute_tables
// 7654 3210
// |||| ||++- Color bits 3-2 for top left quadrant of this byte
// |||| ++--- Color bits 3-2 for top right quadrant of this byte
// ||++------ Color bits 3-2 for bottom left quadrant of this byte
// ++-------- Color bits 3-2 for bottom right quadrant of this byte


// Renders the building on the map and copies data to Nametable B
void _place_house(void) {
	uint map_index;
	type new_palette_block;
	Building building;

	// Check if the player has enough money
	if(net_worth < 50 * choice_pointer_x_id + 50) return;
	net_worth -= 50 * choice_pointer_x_id + 50;

	// Check appropriate kind of building
	building.location = CURSOR_TO_MAP_INDEX;
	building.kind = (choice_pointer_y_id << 2) | choice_pointer_x_id;
	building.b_data = (IS_LIVING_BUILDING(choice_pointer_y_id << 2) ? 150 : 1) * (choice_pointer_x_id + 1);
	append(buildings, building);

//  Index on map array
	map_index = CURSOR_TO_MAP_INDEX;
	tmp_3 = choice_pointer_y_id;

//	Maybe write bulding data in the map array
	map[map_index + 0] = BUILDINGS_[tmp_3][4 * 0 + 2];
	map[map_index + 1] = BUILDINGS_[tmp_3][4 * 1 + 2];
	map[map_index + 32 + 0] = BUILDINGS_[tmp_3][4 * 2 + 2];
	map[map_index + 32 + 1] = BUILDINGS_[tmp_3][4 * 3 + 2];

// 	Build the building from the metatile collection
	one_vram_buffer(BUILDINGS_[tmp_3][4 * 0 + 2], NTADR_A(pointer_x >> 3, pointer_y >> 3));
	one_vram_buffer(BUILDINGS_[tmp_3][4 * 1 + 2], NTADR_A((pointer_x >> 3) + 1, pointer_y >> 3));
	one_vram_buffer(BUILDINGS_[tmp_3][4 * 2 + 2], NTADR_A(pointer_x >> 3, (pointer_y >> 3) + 1));
	one_vram_buffer(BUILDINGS_[tmp_3][4 * 3 + 2], NTADR_A((pointer_x >> 3) + 1, (pointer_y >> 3) + 1));

// 	Calculate palette mask
// 	Calculate offset in the 2x2 palette block (Z rule)
	tmp_3 = 2 * ((pointer_x / 16 % 2) + 2 * (pointer_y / 16 % 2));
// 	Mask to clear the building bits
	tmp_5 = ~(3 << tmp_3);

//	Modify pallete for the building
	ppu_off();
	vram_adr(get_at_addr(1,pointer_x,pointer_y));
	vram_read(tmp_arr, 1);
	vram_adr(get_at_addr(0,pointer_x,pointer_y));
	new_palette_block = tmp_arr[0] & tmp_5 | (choice_pointer_x_id << tmp_3);
	vram_put(new_palette_block);
	vram_adr(get_at_addr(1, pointer_x, pointer_y));
	vram_put(new_palette_block);
	ppu_on_all();
}

void _fill_choice_bg(void) {
	uint idx;
	i = 0;
	while(i < CHOICE_WIDTH) {
		if(is_choice_closed) {
			idx = TILES_TO_INDEX(CHOICE_X_TILE + i, choice_idx);
			tmp_3 = map[idx];
			tmp_data[i] = tmp_3;
		} else {
			tmp_3 = choice_idx - CHOICE_START;
			if(tmp_3 > 10) tmp_data[i] = 0;
			else if(tmp_3 % 2 == 0) {
				if(i % 2 == 0) {
					tmp_data[i] = BUILDINGS_[tmp_3 >> 1][4 * 0 + 2];
				} else if(i % 2 == 1) {
					tmp_data[i] = BUILDINGS_[tmp_3 >> 1][4 * 1 + 2];
				}
			} else if(tmp_3 % 2 == 1) {
				if(i % 2 == 0) {
					tmp_data[i] = BUILDINGS_[tmp_3 >> 1][4 * 2 + 2];
				} else if(i % 2 == 1) {
					tmp_data[i] = BUILDINGS_[tmp_3 >> 1][4 * 3 + 2];
				}
			}
		}
		++i;
	}
}

void _process_choice_menu(void) {
	type palette_left, palette_right;
	type buffer[64] = {0};
	if(is_choice_closed) {
		if(choice_idx > CHOICE_START) {
			--choice_idx;
			_fill_choice_bg();
			if(choice_idx == CHOICE_START + 1) {
				ppu_off();
				vram_adr(NAMETABLE_A);
				vram_write(map, 30 * 32);
				vram_adr(get_at_addr(1, 0, 0));
				vram_read(buffer, 64);
				vram_adr(get_at_addr(0, 0, 0));
				vram_write(buffer, 64);
				ppu_on_all();
			}
			multi_vram_buffer_horz(tmp_data, CHOICE_WIDTH, NTADR_A(CHOICE_X_TILE, choice_idx));
		}
	} else {
		uint idx = TILES_TO_INDEX(pointer_x >> 3, pointer_y >> 3);
		if(map[idx] != GRASS_TILE) {
			is_choice_closed = TRUE;
			return;
		}
		if(choice_idx < CHOICE_END) {
			_fill_choice_bg();
			one_vram_buffer(0b01000100, get_at_addr(0,CHOICE_X_TILE * 8,choice_idx * 8));
			one_vram_buffer(0b11101110, get_at_addr(0,CHOICE_X_TILE * 8 + 32,choice_idx * 8));
			multi_vram_buffer_horz(tmp_data, CHOICE_WIDTH, NTADR_A(CHOICE_X_TILE, choice_idx));
			++choice_idx;
		}
	}
}



// ==============================================
// Fire Event

type is_fire_event = FALSE;

ctype FIRE_TILE = 0xD6;

typedef struct Fire {
	// Building to fire
	Building building;
	// Time in ticks to burn left
	type fire_time;
} Fire;

// Actual list of burining buildings
ListN(Fire, 10) fires;

type fire_idx = 0;

type is_update_palette = FALSE;

uint palette_update_idx = 0;

void update_palette() {
	type tmp = 0;
	type x = (palette_update_idx % WIDTH_IN_TILES) * 8;
	type y = (palette_update_idx / WIDTH_IN_TILES) * 8;
	if (!is_update_palette) return;
	is_update_palette = FALSE;
	vram_adr(get_at_addr(1, x, y));
	vram_read(&tmp, 1);
	tmp = UPDATE_PALETTE_BLOCK(tmp, 0b11, x, y);
	// tmp = 0b00000000;
	// vram_adr(get_at_addr(1, x, y));
	// vram_put(tmp);
	// vram_adr(get_at_addr(0, x, y));
	// vram_adr(NTADR_A(10, 10));
	// vram_put(tmp);
	one_vram_buffer(tmp, get_at_addr(1, x, y));
	one_vram_buffer(tmp, get_at_addr(0, x, y));
}

void demolish(type x, type y, type idx) {
	net_worth += ((buildings.data[idx].kind & 0b11) * 50 + 50) >> 2;

	remove(buildings, idx);
	map[PIXELS_TO_INDEX(x, y)] = GRASS_TILE;
	map[PIXELS_TO_INDEX(x, y) + 1] = GRASS_TILE;
	map[PIXELS_TO_INDEX(x, y) + WIDTH_IN_TILES] = GRASS_TILE;
	map[PIXELS_TO_INDEX(x, y) + WIDTH_IN_TILES + 1] = GRASS_TILE;

	one_vram_buffer(GRASS_TILE, NTADR_A(x >> 3, y >> 3));
	one_vram_buffer(GRASS_TILE, NTADR_A((x >> 3) + 1, y >> 3));
	one_vram_buffer(GRASS_TILE, NTADR_A(x >> 3, (y >> 3) + 1));
	one_vram_buffer(GRASS_TILE, NTADR_A((x >> 3) + 1, (y >> 3) + 1));

// 	Calculate palette mask
// 	Calculate offset in the 2x2 palette block (Z rule)
	tmp_3 = 2 * ((x / 16 % 2) + 2 * (y / 16 % 2));
// 	Mask to clear the building bits
	tmp_5 = ~(3 << tmp_3);

//	Modify pallete for the building
	ppu_off();
	vram_adr(get_at_addr(1,x,y));
	vram_read(tmp_arr, 1);
	vram_adr(get_at_addr(0,x,y));
	tmp_4 = tmp_arr[0] & tmp_5 | (0b11 << tmp_3);
	vram_put(tmp_4);
	vram_adr(get_at_addr(1, x, y));
	vram_put(tmp_4);
	ppu_on_all();
}

uint fire_event_duration = 1000;

void process_fire(void) {
	type fire_idx;
	Fire fire;
	if(is_fire_event) {
		if (fire_event_duration > 0) --fire_event_duration;
		else if (fires.size == 0) {
			is_fire_event = FALSE;
			fire_event_duration = 1000;
			tick = 0;
			for (i = 0; i < buildings.size; ++i) {
				buildings.data[i].kind &= 0b01111;
			}
			return;
		}
		fire_idx = rand8();
		if(fire_idx < buildings.size && fires.size < 10 && !WAS_IN_FIRE(buildings.data[fire_idx])) {
			type idx = TRUE;
			for (i = 0; i < fires.size; ++i) {
				if (fires.data[i].building.location == buildings.data[fire_idx].location) {
					idx = FALSE;
					break;
				}
			}
			if (idx) {
				buildings.data[fire_idx].kind |= 0b10000;
				fire.building = buildings.data[fire_idx];
				fire.fire_time = rand8();
				append(fires, fire);
			}
		}

		for(i = 0; i < fires.size; ++i) {
			oam_spr(fires.data[i].building.location % WIDTH_IN_TILES * 8 + 4, fires.data[i].building.location / WIDTH_IN_TILES * 8 + 4, FIRE_TILE + (fires.data[i].fire_time % 20 > 10), 0b00000010);
			if (fires.data[i].fire_time > 0) {
				--fires.data[i].fire_time;
			} else {
				uint loc = fires.data[i].building.location;
				type x = loc % WIDTH_IN_TILES;
				type y = loc / WIDTH_IN_TILES;
				remove(fires, i);
				is_update_palette = TRUE;
				palette_update_idx = loc;
				for (tmp_3 = 0; tmp_3 < buildings.size; ++tmp_3) {
					if (buildings.data[tmp_3].location == loc) {
						demolish(x * 8, y * 8, tmp_3);
						break;
					}
				}
			}
		}
	}
}


//=============================================
// Displaying numbers

// length = 10
type number_buffer[] = "0000000000";

ulong divu10(uint n) {
    uint q, r;
    q = (n >> 1) + (n >> 2);
    q += (q >> 4);
    q += (q >> 8);
    q += (q >> 16);
    q >>= 3;
    r = n - (((q << 2) + q) << 1);
    return q + (r > 9);
}

void display_number(uint value, uint address) {
	i = 9;
	while(value > 0) {
		number_buffer[i] = '0' + (value % 10);
		// value = divu10(value);
		value /= 10;
		--i;
	}
	while(i < 100) {
		number_buffer[i] = '0';
		--i;
	}
	multi_vram_buffer_horz(number_buffer, 10, NTADR_A(address, VALUE_HEIGHT));
}

void convert_and_display_population(void) {
	display_number(population, POPULATION_START);
}

void convert_and_display_net_worth(void) {
	display_number(net_worth, NET_WORTH_START);
}

void draw_top_info(void) {
    convert_and_display_population();
    convert_and_display_net_worth();
}

// =============================================

#define EVENT_MENU_X_TILE 4
#define EVENT_MENU_Y_TILE 8
#define EVENT_MENU_WIDTH (WIDTH_IN_TILES - EVENT_MENU_X_TILE * 2)
#define EVENT_MENU_HEIGHT (HEIGHT_IN_TILES - EVENT_MENU_Y_TILE * 2 - 2)

type event_menu_opened = FALSE;

type event_menu_buffer[EVENT_MENU_WIDTH] = {0};

type is_fire_event_found = FALSE;

uint event_appear_time;

void process_event_menu(void) {
	type j = 0;
	type buffer[64] = {0};
	if(event_menu_opened) {
		// event_menu_opened = (tick % 100) > 50;
		event_menu_opened = !(pad_new & PAD_A);
		is_fire_event_found = TRUE;
		is_fire_event = TRUE;
		event_appear_time = rand16() / 9;
		if(event_menu_opened) return;
		ppu_off();
		vram_adr(NAMETABLE_A);
		vram_write(map, 30 * 32);
		vram_adr(get_at_addr(1, 0, 0));
		vram_read(buffer, 64);
		vram_adr(get_at_addr(0, 0, 0));
		vram_write(buffer, 64);
		ppu_on_all();
	}
	event_menu_opened |= !is_fire_event && event_appear_time != 0 && (tick > event_appear_time);
	// event_menu_opened = pad_new & PAD_A;
	if(!event_menu_opened) return;
	ppu_off();
	for(j = 0; j < EVENT_MENU_HEIGHT; ++j) {
		for(i = 0; i < EVENT_MENU_WIDTH; ++i) {
			event_menu_buffer[i] = 0x00;
		}
		vram_adr(NTADR_A(EVENT_MENU_X_TILE, EVENT_MENU_Y_TILE + j));
		vram_write(event_menu_buffer, EVENT_MENU_WIDTH);
		vram_adr(get_at_addr(0, EVENT_MENU_X_TILE * 8, EVENT_MENU_Y_TILE * 8 + j * 8));
		vram_fill(0, EVENT_MENU_WIDTH / 4);
		if(j < events.data[0].len) {
			vram_adr(NTADR_A(EVENT_MENU_X_TILE, EVENT_MENU_Y_TILE + j));
			vram_write(events.data[0].strings[j], events.data[0].lines[j]);
		}
	}

	vram_adr(NTADR_A(EVENT_MENU_X_TILE, EVENT_MENU_Y_TILE + EVENT_MENU_HEIGHT - 1));
	vram_write(STR("Accept (A)"), 10);
	ppu_on_all();
}
// ============================================

// According to the game design rules, none of the pointer blocks is able to be placed on water blocks,
// so we need to check if the on is on water only on one of the 4 corners of the pointer block
type is_pointer_in_water() {
	tmp_3 = map[PIXELS_TO_INDEX(pointer_x, pointer_y)];
	return IS_ON_WATER(tmp_3);
}

void render_map(void) {
    vram_adr(NAMETABLE_A);
    vram_write(map, 30 * 32 + 64);
}

// ---- Input Processing ----

#define processCursorButtonPlus(button, cursor, menu_cursor, menu_cursor_bound) \
	if(pad_new & button) { \
		if(is_choice_closed) { \
			cursor += 16; \
			if(is_pointer_in_water()) cursor -= 16; \
		} else if (choice_idx == CHOICE_END && menu_cursor < menu_cursor_bound) { \
			++menu_cursor; \
		} \
	}

#define processCursorButtonMinus(button, cursor, menu_cursor, menu_cursor_bound) \
	if(pad_new & button) { \
		if(is_choice_closed) { \
			cursor -= 16; \
			if(is_pointer_in_water()) cursor += 16; \
		} else if (choice_idx == CHOICE_END && menu_cursor > menu_cursor_bound) { \
			--menu_cursor; \
		} \
	}

// ---- Main Loop ----

ulong commerce_buffer = 0;

type palette_buffer[64] = {0};

uint old_adr = 0;
type x = 0;

void nmi_handler(void) {
	// if((*(type*)0x2002 & 0x10000000) == 1) return;
	// old_adr = *(uint*)0x2006;
	// // // vram_read(&i, 1);
	// // update_palette();
	// vram_adr(old_adr);
}

type music_counter = 0;
type frame_counter = 0;

type industry_mul = 10;

void main(void){
	ulong net_worth_per_tick;

	// event_appear_time = 5000;

	init_events();

	music_play(0);

	// Setup PPU
	ppu_off();

	// Setup Palette
    pal_bg(palette);
    pal_spr(palette);

	render_map();
	vram_adr(NAMETABLE_B);
    vram_write(map, 30 * 32 + 64);

	// Paint Population and Net Worth
	vram_adr(get_at_addr(0,0x20,0));
    vram_fill(0b11110000, 2);
    vram_put(0b11111100);
	vram_adr(get_at_addr(0,0x80,0));
    vram_put(0b11110011);
    vram_fill(0b11110000, 2);
	vram_adr(get_at_addr(1,0x20,0));
    vram_fill(0b11110000, 2);
    vram_put(0b11111100);
	vram_adr(get_at_addr(1,0x80,0));
    vram_put(0b11110011);
    vram_fill(0b11110000, 2);

	// Draw misc stuff
    set_vram_buffer();
    multi_vram_buffer_horz(POPULATION_TEXT, POPULATION_LENGTH, NTADR_A(POPULATION_START, TEXT_HEIGHT));
    multi_vram_buffer_horz(NET_WORTH_TEXT, NET_WORTH_LENGTH, NTADR_A(NET_WORTH_START, TEXT_HEIGHT));
	for(i = 0; i < POPULATION_LENGTH; ++i) map[POPULATION_START + i] = POPULATION_TEXT[i];
	for(i = 0; i < NET_WORTH_LENGTH; ++i) map[NET_WORTH_START + i] = NET_WORTH_TEXT[i];
    render_pointer();
    ppu_on_all();

	nmi_set_callback(nmi_handler);

    while(1) {
        ppu_wait_nmi();
		
		oam_clear();

        pad1 = pad_poll(0);
        pad_new = get_pad_new(0);

		if(pad_new && event_appear_time == 0) {
			// 	2-3 Minutes
			seed_rng();
			event_appear_time = rand16() / 9;
			// event_appear_time = 60;
			tick = 0;
		}

		processCursorButtonMinus(PAD_LEFT, pointer_x, choice_pointer_x_id, 0);
		processCursorButtonPlus(PAD_RIGHT, pointer_x, choice_pointer_x_id, 3);
		processCursorButtonMinus(PAD_UP, pointer_y, choice_pointer_y_id, 0);
		processCursorButtonPlus(PAD_DOWN, pointer_y, choice_pointer_y_id, 4);

        if(pad_new & PAD_A) {
			if(!event_menu_opened) {
				tmp_1 = TRUE;
				if(is_fire_event) {
					for (i = 0; i < fires.size; ++i) {
						if (fires.data[i].building.location == CURSOR_TO_MAP_INDEX) {
							if (net_worth >= (fires.data[i].building.kind & 0b11) * 25 + 25) {
								net_worth -= (fires.data[i].building.kind & 0b11) * 25 + 25;
								remove(fires, i);
							}
							tmp_1 = FALSE;
							break;
						}
					}
				}
				if (tmp_1) {
					if(!is_choice_closed) _place_house();
					is_choice_closed = choice_idx > 10;
				}
			}
        }

		if(pad_new & PAD_B) {
			if(!is_choice_closed) is_choice_closed = TRUE;
			else {
				if(map[CURSOR_TO_MAP_INDEX] != GRASS_TILE) {
					for(i = 0; i < buildings.size; ++i) {
						if(buildings.data[i].location == CURSOR_TO_MAP_INDEX) {
							for(tmp_3 = 0; tmp_3 < fires.size; ++tmp_3) {
								if(fires.data[tmp_3].building.location == buildings.data[i].location) {
									remove(fires, tmp_3)
									break;
								}
							}
							demolish(pointer_x, pointer_y, i);
							break;
						}
					}
				}
			}
		}

		_process_choice_menu();

		net_worth_per_tick = 0;

		// Count in-game variables
		for(i = 0; i < buildings.size; ++i) {
			if ((IS_LIVING_BUILDING(buildings.data[i].kind))) {
				if (buildings.data[i].b_data > 0 && population_buffer > 60) {
					--buildings.data[i].b_data;
					++population;
				}
			} else if (IS_COMMERCIAL_BUILDING(buildings.data[i].kind)) {
				net_worth_per_tick += buildings.data[i].b_data;
			} else {
				industry_mul += buildings.data[i].b_data;
			}
		}
		if (population_buffer > 60) {
			population_buffer -= 60;
		}
		++population_buffer;

		// Count net worth
		commerce_buffer += net_worth_per_tick;
		net_worth_per_tick = commerce_buffer / 60;
		commerce_buffer -= net_worth_per_tick * 60;
		net_worth += net_worth_per_tick * (population / 100) * industry_mul / 10;

		industry_mul = 10;

		// process_event_menu();
		// is_fire_event = TRUE;
		process_event_menu();
		draw_top_info();
		process_fire();
		render_pointer();

		++tick;

		++frame_counter;

		if(frame_counter == 60) {
			frame_counter = 0;
			++music_counter;
		}
		if(music_counter == 120) {
			music_counter = 0;
			music_play(0);
		}
    }
}