#include "speech.h"

#include "globals.h"
#include "hardware.h"

/**
 * Draw the speech bubble background.
 */
static void draw_speech_bubble();

/**
 * Erase the speech bubble.
 */
static void erase_speech_bubble();

/**
 * Draw a single line of the speech bubble.
 * @param line The text to display
 * @param which If TOP, the first line; if BOTTOM, the second line.
 */
#define TOP    0
#define BOTTOM 1
static void draw_speech_line(const char* line, int which);

/**
 * Delay until it is time to scroll.
 */
static void speech_bubble_wait();

void draw_speech_bubble()                                                       // creates black rectangle used as bubble for speech
{
    uLCD.filled_rectangle(3,94,128,114,BLACK);
}

void erase_speech_bubble()                                                      // clears speech bubble after text written
{
    uLCD.filled_rectangle(3,94,128,114,BLACK);
}

void draw_speech_line(const char* line, int which)                              // prints speech line
{
    if(!which) uLCD.locate(1,12);
    if(which) uLCD.locate(1,13);
    if(line) uLCD.printf("%s",line);
}

void speech_bubble_wait()                                                       // waits for 1500ms
{
    wait_ms(1500);
}

void speech(const char* line1, const char* line2)                               // uses bottom portion of map area to display 2 lines of text at a time
{
    draw_speech_bubble();                                                       // make room for bubble
    draw_speech_line(line1, TOP);                                               // print line1
    draw_speech_line(line2, BOTTOM);                                            // print line2
    speech_bubble_wait();                                                       // wait so player can read
    erase_speech_bubble();                                                      // clear bubble
}

void long_speech(const char* lines[], int n)                                    // not used
{
}
