/*
 * Copyright (C) 2011-2013 Pedro H. Penna <pedrohenriquepenna@gmail.com>
 *
 * console.c - Console device driver
 */

#include <asm/io.h>
#include <nanvix/const.h>
#include <nanvix/dev.h>
#include <sys/types.h>
#include <stdint.h>
#include "tty.h"

/* Video colors (black background). */
#define BLACK         0x0 /* Black .           */
#define BLUE          0x1 /* Blue.             */
#define GREEN         0x2 /* Green.            */
#define CYAN          0x3 /* Cyan.             */
#define RED           0x4 /* Red.              */
#define MAGENTA       0x5 /* Magenta.          */
#define BROWN         0x6 /* Brown.            */
#define LIGHT_GREY    0x7 /* Light grey.       */
#define DARK_GREY     0x8 /* Dark grey.        */
#define LIGHT_BLUE    0x9 /* Light blue.       */
#define LIGHT_GREEN   0xa /* Light green.      */
#define LIGHT_CYAN    0xb /* Light cyan.       */
#define LIGHT_RED     0xc /* Light red.        */
#define LIGHT_MAGENTA 0xd /* Light magenta     */
#define YELLOW        0xe /* Yellow.           */
#define WHITE         0xf /* White.            */

/* Video specifications (Text mode). */
#define VIDEO_ADDR  0xb8000 /* Video memory address. */
#define VIDEO_WIDTH      80 /* Video width.          */
#define VIDEO_HIGH       25 /* Video high.           */

/* Video registers. */
#define VIDEO_CRTL_REG 0x3d4 /* Video control register. */
#define VIDEO_DATA_REG 0x3d5 /* Video data register.    */
	
/* Video control offset registers. */
#define VIDEO_HTOT 0x00 /* Horizontal total.               */
#define VIDEO_HDEE 0x01 /* Horizontal display enabled end. */
#define VIDEO_SHB  0x02 /* Start horizontal blanking.      */
#define VIDEO_EHB  0x03 /* End Horizontal blanking.        */
#define VIDEO_SHRP 0x04 /* Start horizontal retrace pulse. */
#define VIDEO_EHRP 0x05 /* End horizontal retrace pulse.   */
#define VIDEO_VTR  0x06 /* Vertical total.                 */
#define VIDEO_OVRF 0x07 /* Overflow.                       */
#define VIDEO_PRS  0x08 /* Preset row scan.                */
#define VIDEO_MSL  0x09 /* Maximum scan line.              */
#define VIDEO_CS   0x0a /* Cursor start.                   */
#define VIDEO_CE   0x0b /* Cursor end.                     */
#define VIDEO_SAH  0x0c /* Start address high.             */
#define VIDEO_SAL  0x0d /* Start address low.              */
#define VIDEO_CLH  0x0e /* Cursor location high.           */
#define VIDEO_CLL  0x0f /* Cursor location low.            */
#define VIDEO_RSR  0x10 /* Vertical retrace start.         */
#define VIDEO_RSE  0x11 /* Vertical retrace end.           */
#define VIDEO_VDEE 0x12 /* Vertical display-enable end.    */
#define VIDEO_OFF  0x13 /* Offset.                         */
#define VIDEO_ULOC 0x14 /* Underline location.             */
#define VIDEO_SVB  0x15 /* Start vertical blanking.        */
#define VIDEO_EVB  0x16 /* End vertical blanking.          */
#define VIDEO_CMC  0x17 /* CRT mode control.               */
#define VIDEO_LCMP 0x18 /* Line compare.                   */

/*
 * Console cursor.
 */
PRIVATE struct 
{
	int x; /* Horizontal axis position. */
	int y; /* Vertical axis position.   */
} cursor;

/* Video memory.*/
PRIVATE uint16_t *video = (uint16_t*)VIDEO_ADDR;

/*
 * Moves the hardware console cursor.
 */
PRIVATE void cursor_move()
{
	word_t cursor_location = cursor.y*VIDEO_WIDTH + cursor.x;
	
	outputb(VIDEO_CRTL_REG, VIDEO_CLH);
	outputb(VIDEO_DATA_REG, (byte_t) ((cursor_location >> 8) & 0xFF));
	outputb(VIDEO_CRTL_REG, VIDEO_CLL);
	outputb(VIDEO_DATA_REG, (byte_t) (cursor_location & 0xFF));
}

/*
 * Scrolls down the console by one row.
 */
PRIVATE void console_scrolldown()
{
	uint16_t *p;
	
	/* Pull lines up. */
	for (p = video; p < (video + (VIDEO_HIGH-1)*(VIDEO_WIDTH)); p++)
		*p = *(p + VIDEO_WIDTH);
	
	/* Blank last line. */
	for (; p < video + VIDEO_HIGH*VIDEO_WIDTH; p++)
		*p = (BLACK << 8) | (' ');
		
	/* Set cursor position. */
	cursor.x = 0; cursor.y = VIDEO_HIGH - 1;
}

/*
 * Outputs a colored ASCII character on the console device.
 */
PRIVATE void console_put(uint8_t ch, uint8_t color)
{	
	/* Parse character. */
    switch (ch)
    {
        /* New line. */
        case '\n':
            cursor.y++;
            cursor.x = 0;
            break;
            
        /* Tabulation. */
        case '\t':
            /* FIXME. */
            cursor.x += 4 - (cursor.x & 3);
            break;
            
        /* Backspace. */
        case '\b':
            if (cursor.x > 0)
                cursor.x--;
            else if (cursor.y > 0)
            {
                cursor.x = VIDEO_WIDTH - 1;
                cursor.y--;
            }
            video[cursor.y*VIDEO_WIDTH +cursor.x] = (color << 8) | (' ');
            break;			
        
        /* Any other. */
        default:
            video[cursor.y*VIDEO_WIDTH +cursor.x] = (color << 8) | (ch);
            cursor.x++;
            break;
    }

    /* Set cursor position. */
    if (cursor.x >= VIDEO_WIDTH)
    {
        cursor.x = 0;
        cursor.y++;
    }
    if (cursor.y >= VIDEO_HIGH)
        console_scrolldown();
    cursor_move();
}

/*
 * Clears the console.
 */
PRIVATE void console_clear()
{
	uint16_t *p;
	
	/* Blank all lines. */
	for (p = video; p < (video + VIDEO_HIGH*VIDEO_WIDTH); p++)
		*p = (BLACK << 8) | (' ');
	
	/* Set console cursor position. */
	cursor.x = cursor.y = 0;
	cursor_move();
}

/*
 * Flushes a buffer on the console device.
 */
PUBLIC void console_write(struct kbuffer *buffer)
{
	uint8_t ch;
	
	/* Outputs all characters. */
	while (!KBUFFER_EMPTY((*buffer)))
	{ 
		KBUFFER_GET((*buffer), ch);
	
		console_put(ch, WHITE);
	}
}

/*
 * Initializes the console driver.
 */
PUBLIC void console_init()
{
	byte_t value;
	
	/* Set cursor shape. */
	outputb(VIDEO_CRTL_REG, VIDEO_CS);
	value = inputb(VIDEO_DATA_REG);
	outputb(VIDEO_CRTL_REG, VIDEO_CS);
	outputb(VIDEO_DATA_REG, (byte_t) (value & 0xe0));
	
	/* Clear the console. */
	console_clear();
}