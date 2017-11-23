/*
 * High level TFT functions
 * Author:  LoBo 04/2017, https://github/loboris
 * 
 */

#ifndef _MGOS_TFT_TOUCH_H_
#define _MGOS_TFT_TOUCH_H_

#include <stdlib.h>
#include "tftspi.h"


//==========================================================================================
// ==== Global variables ===================================================================
//==========================================================================================
extern uint8_t   orientation;		// current screen orientation
extern uint16_t  font_rotate;   	// current font font_rotate angle (0~395)
extern uint8_t   font_transparent;	// if not 0 draw fonts transparent
extern uint8_t   font_forceFixed;   // if not zero force drawing proportional fonts with fixed width
extern uint8_t   font_buffered_char;
extern uint8_t   font_line_space;	// additional spacing between text lines; added to font height
extern uint8_t   text_wrap;         // if not 0 wrap long text to the new line, else clip
extern color_t   _fg;            	// current foreground color for fonts
extern color_t   _bg;            	// current background for non transparent fonts
extern dispWin_t dispWin;			// display clip window
extern float	  _angleOffset;		// angle offset for arc, polygon and line by angle functions
extern uint8_t	  image_debug;		// print debug messages during image decode if set to 1

extern Font cfont;					// Current font structure

extern int	TFT_X;					// X position of the next character after TFT_print() function
extern int	TFT_Y;					// Y position of the next character after TFT_print() function

extern uint32_t tp_calx;			// touch screen X calibration constant
extern uint32_t tp_caly;			// touch screen Y calibration constant
// =========================================================================================



// ===== PUBLIC FUNCTIONS =========================================================================


/*
 * Get the touch panel coordinates.
 * The coordinates are adjusted to screen orientation if raw=0
 *
 * Params:
 * 		x: pointer to X coordinate
 * 		y: pointer to Y coordinate
 * 	  raw: if 0 returns calibrated screen coordinates; if 1 returns raw touch controller coordinates
 *
 * Returns:
 * 		0	if touch panel is not touched; x=y=0
 * 		>0	if touch panel is touched; x&y are the valid coordinates
 */
//----------------------------------------------
int TFT_read_touch(int *x, int* y, uint8_t raw);


bool TFT_Touch_intr_init(void);

/**
 * @brief MGOS lib init
 */
bool mgos_mos_lobo_tft_init(void);


#endif
