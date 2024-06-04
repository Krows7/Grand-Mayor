#include "core.h"

const unsigned char water[]={

	0xc0, 0xc1, 0xc2, 0xc3,
	0xc4, 0xc5, 0xc6, 0xc7,
	0xc8, 0xc9, 0xca, 0xcb,
	0xd0, 0xd1, 0xd2, 0xd3,
	0xd5
};




const unsigned char pointer[]={

	  0,  0,0xcc,2,
	  8,  0,0xcd,2,
	  0,  8,0xce,2,
	  8,  8,0xcf,2,
	0x80

};

const unsigned char choice_pointer[]={

	  0,  0,0xcc,0,
	  8,  0,0xcd,0,
	  0,  8,0xce,0,
	  8,  8,0xcf,0,
	0x80

};

const unsigned char _shop[]={

	  0,  0,0x80,0,
	  8,  0,0x81,0,
	  0,  8,0x90,0,
	  8,  8,0x91,0,
	0x80

};

const unsigned char _house1[]={

	  0,  0,0x82,0,
	  8,  0,0x83,0,
	  0,  8,0x92,0,
	  8,  8,0x93,0,
	0x80

};

const unsigned char _house2[]={

	  0,  0,0x84,0,
	  8,  0,0x85,0,
	  0,  8,0x94,0,
	  8,  8,0x95,0,
	0x80

};

const unsigned char _factory[]={

	  0,  0,0x86,0,
	  8,  0,0x87,0,
	  0,  8,0x96,0,
	  8,  8,0x97,0,
	0x80

};

const unsigned char _gas_station[]={

	  0,  0,0xae,0,
	  8,  0,0xaf,0,
	  0,  8,0xbe,0,
	  8,  8,0xbf,0,
	0x80

};


// const unsigned char palette[16]={ 0x0f,0x00,0x10,0x30,0x0f,0x01,0x21,0x31,0x0f,0x06,0x16,0x26,0x0f,0x21,0x19,0x29 };
unsigned char palette[16]={ 0x0f,0x00,0x10,0x30,0x0f,0x0f,0x01,0x21,0x0f,0x06,0x27,0x37,0x0f,0x21,0x19,0x29 };
// const unsigned char palette[16]={ 0x0f,0x00,0x10,0x30,0x11,0x0f,0x01,0x21,0x21,0x06,0x27,0x37,0x31,0x21,0x19,0x29 };

// Metatile Collection
ctype* BUILDINGS_[] = {_shop, _house1, _house2, _factory, _gas_station};

ctype FIELD_TILE = 0xD4;

typedef struct Event {
	ctype** strings;
	ctype* lines;
	type len;
} Event;

List(Event) events;

Event FIRE;

const ctype* FIRE_STRINGS[] = {STR("Fire!"), 
								STR("Someone forgot to turn"), 
								STR("off the stove!"), 
								STR("The raging flames spread"), 
								STR("to the buildings around"), 
								STR("If you"), 
								STR("don't put a house out,"),
								STR("it will burn down within"), 
								STR("20 seconds!")};
const ctype FIRE_LINES[] = {5, 22, 14, 24, 23, 6, 22, 24, 11};
const type FIRE_LEN = 9;

void init_events(void) {
	FIRE.strings = FIRE_STRINGS;
	FIRE.lines = FIRE_LINES;
	FIRE.len = FIRE_LEN;

	append(events, FIRE);
}