#include "lib/nesdoug.h"
#include "lib/neslib.h"

// 1 byte
typedef unsigned char type;
typedef const unsigned char ctype;
// 2 bytes
typedef unsigned int uint;
// 4 bytes
typedef unsigned long ulong;

// ---- String Utils ----

#define STR(x) (unsigned char*) x

// ---- List Utils ----

type LIST_VAR_LAST = 0;
type LIST_VAR_FIRST = 0;
type LIST_VAR_IDX = 0;

#define LIST_MAX_SIZE 50
#define List(clz) struct {clz data[LIST_MAX_SIZE]; type size;}
#define ListN(clz, amount) struct {clz data[amount]; type size;}
#define append(array, value) \
    LIST_VAR_LAST = LIST_MAX_SIZE; \
    LIST_VAR_FIRST = LIST_MAX_SIZE; \
    LIST_VAR_IDX = LIST_MAX_SIZE; \
    if (array.size < LIST_MAX_SIZE) { \
        array.data[array.size] = value; \
        ++array.size; \
    } else { \
        for (;LIST_VAR_IDX > 0; --LIST_VAR_IDX) { \
            if (&array.data[LIST_VAR_IDX - 1] != 0) { \
                if(LIST_VAR_LAST == LIST_MAX_SIZE) LIST_VAR_LAST = LIST_VAR_IDX; \
            } else { \
                LIST_VAR_FIRST = LIST_VAR_IDX - 1; \
            } \
        } \
        if (LIST_VAR_FIRST != LIST_MAX_SIZE) array.data[LIST_VAR_FIRST] = value; \
        if (LIST_VAR_LAST != LIST_MAX_SIZE) array.size = LIST_VAR_LAST; \
    }
#define appendAtEnd(array, value) array.data[array.size] = value; ++array.size;
#define erase(array, index) &array.data[index] = 0;
#define remove(array, index) \
    for (LIST_VAR_IDX = index + 1; LIST_VAR_IDX < array.size; ++LIST_VAR_IDX) { \
        array.data[LIST_VAR_IDX - 1] = array.data[LIST_VAR_IDX]; \
    } \
    --array.size;

// ---- Debug Utils ----

#define i tmp_2

type tmp_1 = 0;
type tmp_2 = 0;

type huy[] = "000";

// Renders decimal number to the screen at 5x20 Tile
void _debug_print(type num) {
	tmp_1 = num;
    i = 2;
    while(tmp_1 > 0) {
        huy[i] = '0' + (tmp_1 % 10);
        tmp_1 /= 10;
        --i;
    }
    while(i < 100) {
        huy[i] = '0';
        --i;
    }
	multi_vram_buffer_horz(huy, 3, NTADR_A(5, 20));
}

// Renders hex number to the screen at 5x20 Tile
void _debug_hex(type num) {
	tmp_1 = num;
    i = 2;
    while(tmp_1 > 0) {
        if((tmp_1 % 16) < 10) huy[i] = '0' + (tmp_1 % 16);
		else huy[i] = 'A' + (tmp_1 % 16) - 10;
        tmp_1 /= 16;
        --i;
    }
    while(i < 100) {
        huy[i] = '0';
        --i;
    }
	multi_vram_buffer_horz(huy, 3, NTADR_A(5, 20));
}