/* 
 *   #####         #
 *     #       #  ###     #  ###
 *     #    #      #   #     #
 *     #   ##  #   #  ##  #   # 
 *     #   #   #   #  #   #    #
 *     #   #   #   #  #   #  ###
 *
 *	Author:					Jegge (C) 2009/2010 - Some Rights Reserved
 *
 * 	License:				Released under Creative Commons CC-by-nc-sa license.
 *
 *	Hardware Requirements:	Mignonette v1.0
 *
 *  Instructions:			- C -> Move left
 *							- D -> Move right
 *							- A -> Rotate
 *							- B -> Pause / Resume
 * 
 *	Note: This source code is licensed under a Creative Commons License, CC-by-nc-sa.
 *		(attribution, non-commercial, share-alike)
 *  	see http://creativecommons.org/licenses/by-nc-sa/3.0/ for details.			
 *
 *  Credits:	
 * 		- mitch altman 			- for the munch source code which served as template
 *      - rolf van widenfelt 	- for the munch source code which served as template
 *
 *  Release Notes:
 *    12/29/2009 - 0.01 -	Initial release @ 26C3
 *	  12/31/2009 - 0.02 -	First Bugfix release. 
 *								- Better Random Number Generator, moved into miggl.c
 *								- Delay stuff moved into miggl.c
 *	  01/04/2010 - 0.03 -	Another small set of fixes
 *								- Random Number is now 8 bits wide instead of 32
 *								- Entries in CornerStones[] are now in correct order
 *								- swapped some types (int to uint8_t)
 *    01/04/2010 - 0.031 		- (rolf) fix ^M (newline) chars in source files (tris2.c, miggl.[ch])
 *	  01/05/2010 - 0.04		Should be the final version for now
 *								- Random numbers 32 bit again :) 
 *								- Added README.txt
 */

#include <inttypes.h>
#include <avr/io.h>			/* this takes care of definitions for our specific AVR */
#include <avr/pgmspace.h>	/* needed for printf_P, etc */
#include <avr/interrupt.h>	/* for interrupts, ISR macro, etc. */
#include <stdio.h>			// for sprintf, etc.
#include <stdlib.h>			// for abs(), etc.
#include "mydefs.h"
#include "iodefs.h"
#include "miggl.h"			/* Mignonette Game Library */

#define UP 		0x01
#define LEFT 	0x02
#define MIDDLE  0x04
#define RIGHT 	0x08
#define DOWN 	0x10
// korobeneiki - at least something similiar 
byte IntroSong[] = {
	N_E4,N_QUARTER,
	N_B3,N_8TH,
	N_C4,N_8TH,
	N_D4,N_QUARTER,
	N_C4,N_8TH,
	N_B3,N_8TH,
	N_A3,N_QUARTER,
	N_A3,N_8TH,
	N_C4,N_8TH,
	N_E4,N_QUARTER,
	N_D4,N_8TH,
	N_C4,N_8TH,
	N_B3,N_QUARTER,
	N_B3,N_8TH,
	N_C4,N_8TH,
	N_D4,N_QUARTER,
	N_E4,N_QUARTER,
	N_C4,N_QUARTER,
	N_A3,N_QUARTER,
	N_A3,N_QUARTER,
	N_D4,N_QUARTER,
	N_F4,N_8TH,
	N_A4,N_QUARTER,
	N_G4,N_8TH,
	N_F4,N_8TH,
	N_E4,N_QUARTER,
	N_C4,N_8TH,
	N_E4,N_QUARTER,
	N_D4,N_8TH,
	N_C4,N_8TH,
	N_B3,N_QUARTER,
	N_B3,N_8TH,
	N_C4,N_8TH,
	N_D4,N_QUARTER,
	N_E4,N_QUARTER,
	N_C4,N_QUARTER,
	N_A3,N_QUARTER,
	N_A3,N_QUARTER,
	N_END
};


// the bitmaps displayed for the intro screen
uint8_t IntroScreenGreen[] = { 0x0E, 0xFA, 0x82, 0xFA, 0x0E };
uint8_t IntroScreenYellow[] = { 0x00, 0x04, 0x7C, 0x04, 0x00 };

// the bitmaps displayed for the game over screen
uint8_t GameOverScreenRed[] = { 0x0E, 0x1A, 0x12, 0x1A, 0x0E };
uint8_t GameOverScreenYellow[] = { 0xA0, 0xA4, 0x40, 0xA4, 0xA0 };

// bitmap for the current stone
uint8_t MaskField[] = { 0x00, 0x00, 0x00, 0x00, 0x00 };

// bitmap for the stacked stones
uint8_t PlayField[] = { 0x00, 0x00, 0x00, 0x00, 0x00 };

// the corner stones
uint8_t CornerStones[] = {
	UP | MIDDLE | RIGHT,
	UP | LEFT | MIDDLE,	
	LEFT | MIDDLE | DOWN,
	MIDDLE | RIGHT | DOWN	
};

// the straight stones
uint8_t StraightStones[] = { 
	UP | MIDDLE | DOWN,
	LEFT | MIDDLE | RIGHT
};


//
// draws bits 2 to 7 into a line of the screen with a given color
//
void draw_bit_line (uint8_t line, uint8_t color, uint8_t bits) {
	uint8_t i;
	setcolor(color);
	for (i = 0; i < XSCREEN; i++) {
		uint8_t m = ((0x02) << i);
		if ((m & bits) == m)
			drawpoint(i, line);
	}
}

//
// draws a whole bitmap into the screen with a given color
//
void draw_bitmap (uint8_t* screen, uint8_t color) {
	uint8_t y;
	for (y = 0; y < YSCREEN; y++)
		draw_bit_line(y, color, screen[y]);			
}

//
// waits for a keypress and restarts music meanwhile if needed
//
void wait_for_anykey (void) {
	while (1) {
		handlebuttons();
		if (ButtonA || ButtonB || ButtonC || ButtonD)
			break;
	}
	ButtonA = ButtonB = ButtonC = ButtonD = 0;
	sleep_ms(250);
}

//
// shows the intro screen and waits for a keypress
//
void show_intro_screen (void) {
	// show bitmap
	cleardisplay();	
	draw_bitmap(IntroScreenGreen, GREEN);
	draw_bitmap(IntroScreenYellow, YELLOW);
	swapbuffers();
	sleep_ms(250);	
	// wait for keypress
	wait_for_anykey();
}

//
// shows the game over screen and waits for a keypress
//
void show_gameover_screen (void) {
	// show bitmap
	cleardisplay();	
	draw_bitmap(GameOverScreenRed, RED);
	draw_bitmap(GameOverScreenYellow, YELLOW);
	swapbuffers();
	sleep_ms(250);	
	// wait for keypress
	wait_for_anykey();
}

// 
// rotates a stone, returns the rotated stone
//
uint8_t rotate_stone (uint8_t stone) {
	// get idx of stone in CornerStones
	int8_t  idx = -1;
	uint8_t j = 0;
	uint8_t isStraight = 0;
	for (j = 0; j < 4; j++) 
		if (CornerStones[j] == stone) {
			idx = j;
			isStraight = 0;
			break;		
		}
	// if not found get idx of stone in StraightStones
	if (idx == -1) 
		for (j = 0; j < 2; j++) 
			if (StraightStones[j] == stone) {
				idx = j;
				isStraight = 1;
				break;		
			}	
	// if we found an idx ... return next idx in array
	if (idx != -1) {
		if (isStraight) 
			return StraightStones[(idx == 1) ? 0 : idx + 1];		
		else 
			return CornerStones[(idx == 3) ? 0 : (idx + 1)];		
	}
	return 0;
}

//
// clears a bitmap
//
void clear_bitmap (uint8_t* screen) {
	uint8_t i = 0;
	for (i = 0; i < 5; i++)
		screen[i] = 0x00;
}

//
// draws a pixel into a bitmap
//
void draw_pixel_to_bitmap (uint8_t* screen, uint8_t x, uint8_t y) {
	if ((x <= XSCREEN) && (y < YSCREEN))
		screen[y] = screen[y] | (0x01 << x);
}

//
// draws a stone into a given bitmap
//
void draw_stone_to_bitmap (uint8_t* screen, uint8_t x, uint8_t y, uint8_t stone) {
	draw_pixel_to_bitmap (screen, x, y);
	if ((stone & UP) == UP)
		draw_pixel_to_bitmap  (screen, x - 1, y);
	if ((stone & DOWN) == DOWN)
		draw_pixel_to_bitmap  (screen, x + 1, y);
	if ((stone & LEFT) == LEFT)
		draw_pixel_to_bitmap  (screen, x, y + 1);
	if ((stone & RIGHT) == RIGHT)
		draw_pixel_to_bitmap  (screen, x, y - 1);
}	

//
// finds the next complete line in the playfield, starting out from the bottom
// returns the index of the line
//
int8_t get_complete_line () {
	int8_t i = 0, j = 0, k = 0;

	for (j = XSCREEN - 1; j >= 0; j--) {
		k = 0;	
		for (i = 0; i < 5; i++)
			if ((PlayField[i] & (0x02 << j)) == (0x02 << j))
				k++;
		if (k == 5)
			return j;
	}
	return -1;
}

//
// clears a line in the playfield, by first blinking it, then removing the line
// and finally shifting the stuff above down a line
//
void clear_line (int8_t line) {
	uint8_t i = 0;
	uint8_t mask_lower = 0xFF << (line + 2);
	uint8_t mask_upper = 0xFF >> (XSCREEN - line);

	// make line blink...
	setcolor(YELLOW);
	drawfilledrect(line, 0, line, YSCREEN - 1);
	swapbuffers();
	sleep_ms(25);
	setcolor(GREEN);
	drawfilledrect(line, 0, line, YSCREEN - 1);
	swapbuffers();
	sleep_ms(25);
	setcolor(YELLOW);
	drawfilledrect(line, 0, line, YSCREEN - 1);
	swapbuffers();
	sleep_ms(25);
	setcolor(GREEN);
	drawfilledrect(line, 0, line, YSCREEN - 1);
	swapbuffers();
	sleep_ms(25);
		
	// remove line from bitmap and drop the upper part one down
	for (i = 0; i < 5; i++) 
		PlayField[i] = ((PlayField[i] & mask_upper) << 0x01) | (PlayField[i] & mask_lower);

	// update the display
	cleardisplay();
	draw_bitmap(PlayField, GREEN);
	swapbuffers();
	sleep_ms(50);
}

//
// checks if two bitmaps overlap, returns 1 if they do or 0 otherwise
//
uint8_t screens_overlap (uint8_t* screen1, uint8_t* screen2) {
	uint8_t i = 0;
	for (i = 0; i < 5; i++)
		if ((screen1[i] & screen2[i]) != 0)
			return 1;
	return 0;
}

//
// checks if a pixel is set in a bitmap, returns 1 if they do or 0 otherwise
//
uint8_t is_field_set (uint8_t* screen, uint8_t x, uint8_t y) {
	return ((screen[y] & (0x01 << x)) == (0x01 << x)) ? 1 : 0;
}

//
// tests wether a stone can be moved in a given direction
// returns 1 if they do or 0 otherwise
//
uint8_t can_move_stone (uint8_t* screen, uint8_t stone, uint8_t x, uint8_t y, uint8_t dir) {
	// stone can be moved, if ...
	uint8_t canmove = 0;
	uint8_t TempField[] = { 0x00, 0x00, 0x00, 0x00, 0x00 };
	int8_t left = ((stone & LEFT) == LEFT) ? (y + 1) : y;
	int8_t right = ((stone & RIGHT) == RIGHT) ? (y - 1) : y;
	int8_t down = ((stone & DOWN) == DOWN) ? (x + 1) : x;

	// ... it  stays within screen bounds
	if (((dir & LEFT) == LEFT) && (left <= YSCREEN - 1))
		canmove |= LEFT;
	if (((dir & RIGHT) == RIGHT) && (right >= 0))
		canmove |= RIGHT;
	if (((dir & DOWN) == DOWN) && (down <= XSCREEN))
		canmove |= DOWN;
	// UP is not needed

	// ... it does not overlap
	draw_stone_to_bitmap(TempField, x, y, stone);
	if (screens_overlap(screen, TempField))
		canmove &= ~dir;

	return ((canmove & dir) == dir) ? 1 : 0;
}

//
// tests wether a stone can be rotated
// returns 1 if they do or 0 otherwise
//
uint8_t can_rotate_stone (uint8_t* screen, uint8_t stone, uint8_t x, uint8_t y) {
	// stone can be rotated, if ...
	uint8_t s = rotate_stone (stone);
	uint8_t TempField[] = { 0x00, 0x00, 0x00, 0x00, 0x00 };	
	int8_t left = ((s & LEFT) == LEFT) ? (y + 1) : y;
	int8_t right = ((s & RIGHT) == RIGHT) ? (y - 1) : y;
	int8_t down = ((s & DOWN) == DOWN) ? (x + 1) : x;
	
	// ... it  stays within screen bounds
	if (left > YSCREEN - 1)
		return 0;
	if (right < 0)
		return 0;
	if (down > XSCREEN)
		return 0;

	// ... it does not overlap
	draw_stone_to_bitmap(TempField, x, y, s);
	if (screens_overlap(screen, TempField))
		return 0;
	return 1;
}

//
// gets a random stone and returns it
//
uint8_t get_random_stone () {
	uint8_t idx = (uint8_t) nextrandom(6);
	if (idx < 4)
		return CornerStones[idx];
	return StraightStones[idx - 4];
}

// flashes the screen n times
void flash_screen (uint8_t n) {
	uint8_t i, x, y;

	for (i = 0; i < n; i++) {
		for (x = 0; x < XSCREEN; x++) 	
			for (y = 0; y < YSCREEN; y++) {
				setcolor ((readpixel(x, y) == BLACK) ? YELLOW : RED);
				drawpoint(x ,y);
			}
		swapbuffers();
		sleep_ms(50);
		for (x = 0; x < XSCREEN; x++) 	
			for (y = 0; y < YSCREEN; y++) {
				setcolor ((readpixel(x, y) == YELLOW) ? BLACK: GREEN);
				drawpoint(x, y);
			}
		swapbuffers();
		sleep_ms(50);
	}
}

// 
// the gameloop for the actual gameplay
//
void gameloop (void) {

	int8_t stonex = 0, stoney = 2;
	int8_t falltime = 9;
	int8_t falltimemax = 9;
	int8_t  completeline = -1;
	uint8_t stone = get_random_stone();
	uint8_t solvedlines = 0;
	uint8_t levelcount = 0;

	clear_bitmap(MaskField);
	clear_bitmap(PlayField);

	while (1) {

		// draw display
		cleardisplay();		
		clear_bitmap(MaskField);
		draw_stone_to_bitmap(MaskField, stonex, stoney, stone);
		draw_bitmap(PlayField, GREEN);
		draw_bitmap(MaskField, RED);		

		// handle the button presses
		handlebuttons();

		if (ButtonB) { 				// pause
			sleep_ms(250);
			wait_for_anykey();
		} else if (ButtonA && ButtonAEvent) { 		// rotate
			if (can_rotate_stone(PlayField, stone, stonex, stoney))
				stone = rotate_stone(stone);
				ButtonAEvent = 0;
		} else if (ButtonC) { 		// move left
			if (can_move_stone(PlayField, stone, stonex, stoney + 1, LEFT))
				stoney++;
		} else if (ButtonD) { 		// move right
			if (can_move_stone(PlayField, stone, stonex, stoney - 1, RIGHT))
				stoney--;
		}

		// let stone fall
		falltime--;
		if (falltime <= 0) {
			falltime = falltimemax;
			if (can_move_stone(PlayField, stone, stonex + 1, stoney, DOWN))
				stonex++;
			else {
				draw_stone_to_bitmap(PlayField, stonex, stoney, stone);
				cleardisplay();
				draw_bitmap(PlayField, GREEN);
				swapbuffers();
				
				// if fallen stone is to high and reaches out of the display ... game over
				if (stonex <= 0) 
					return;

				// continue with next stone ...
				stonex = 0;
				stoney = 2; 
				stone = get_random_stone();
				falltime = falltimemax;
			}
		}

		// check if line complete
		completeline = -1;
		while ((completeline = get_complete_line()) != -1) {
			clear_line(completeline);
			solvedlines++;
			sleep_ms(120);
		}

		// we get faster after a while
		if (solvedlines >= 10) { 
			solvedlines = 0;		
			falltimemax = (falltimemax < 1) ? 0 : (falltimemax - 1);
			levelcount++;
			// flash the screen level count times, to  indicate progress...
			flash_screen(levelcount);
		}
		swapbuffers();
	}

}

// 
// tri2s main program
//
int main (void) {
	
	initmiggl();

	setenvelope(128, 32, 60, 32);

	playsong(IntroSong);
	loopsong(1);
	
	while (1) {
		show_intro_screen();
		gameloop();
		show_gameover_screen();
	}
	return 0;
}

