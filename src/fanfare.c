#include <rp6502.h>
#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include "ezpsg.h"
#include "colors.h"
#include "bitmap_graphics.h"

#define CANVAS_W 320
#define CANVAS_H 240

#define CHAR_W 6

#define ASPRITE_SIZE 64
#define LOG_ASPRITE_SIZE 6 // 2^6 = 64

#define ASPRITE_CONFIG_SIZE sizeof(vga_mode4_asprite_t)
#define ASPRITE_CONFIGS 0xF800 // start of affine sprite configs (actually 0x1F800)

#define BREADBOARD_DATA 0x9800 // (actually 0x19800)
#define CIRCUITBOARD_DATA 0xB800
#define PORTABLE_DATA 0xD800

#define BREADBOARD_INDEX 0
#define CIRCUITBOARD_INDEX 1
#define PORTABLE_INDEX 2
#define TOTAL_ASPRITES 3

typedef struct {
    int16_t x_pos;
    int16_t y_pos;
    uint16_t config_ptr;
    uint16_t data_ptr;
} asprite_t;
static asprite_t asprite[TOTAL_ASPRITES];


static uint32_t asprite_timer = 0;
static uint32_t breadboard_asprite_timer_threshold = 200;
static uint32_t circuitboard_asprite_timer_threshold = 400;
static uint32_t portable_asprite_timer_threshold = 600;
static uint32_t reviews_asprite_timer_threshold = 800;

#define wait(duration) (duration)
#define trumpet(note, duration) (-1), (note), (duration)
#define end() (0)

// ----------------------------------------------------------------------------
// Fanfare for the Common Man, by Aaron Copland
// ----------------------------------------------------------------------------
#define bar_0 trumpet(g3, 1), wait(1), \
              trumpet(c4, 1), wait(1), \
              trumpet(g4, 14), wait(14)

#define bar_1 trumpet(c4, 1), wait(1), \
              trumpet(g4, 1), wait(1), \
              trumpet(f4, 10), wait(10), \
              trumpet(a4, 4), wait(4)

#define bar_2 trumpet(f4, 4), wait(4), \
              trumpet(c4, 4), wait(4), \
              trumpet(g3, 8), wait(8)

#define bar_3 trumpet(f3, 1), wait(1), \
              trumpet(a3, 1), wait(1), \
              trumpet(c4, 10), wait(10), \
              trumpet(c4, 1), wait(1), \
              trumpet(e4, 1), wait(1)

#define bar_4 trumpet(g4, 10), wait(10), \
              trumpet(f3, 1), wait(1), \
              trumpet(a3, 1), wait(1), \
              trumpet(c4, 8), wait(8)

#define bar_5 trumpet(e4, 1), wait(1), \
              trumpet(g3, 1), wait(1), \
              trumpet(c3, 18), wait(18)

static const uint8_t song[] = {
    wait(4),
    bar_0,
    bar_1,
    bar_2,
    bar_3,
    bar_4,
    bar_5,
    end()};

// ----------------------------------------------------------------------------
// ----------------------------------------------------------------------------
void ezpsg_instruments(const uint8_t **data)
{
    switch ((int8_t) * (*data)++) // instrument
    {
    case -1: // piano
        ezpsg_play_note(*(*data)++, // note
                        *(*data)++, // duration
                        1,          // release
                        32,         // duty
                        0x33,       // vol_attack
                        0xFD,       // vol_decay
                        0x38,       // wave_release
                        0);         // pan
        break;
    default:
        // The instrumment you just added probably isn't
        // consuming the correct number of paramaters.
        puts("Unknown instrument.");
        exit(1);
    }
}

// ----------------------------------------------------------------------------
// draw title, centered
// ----------------------------------------------------------------------------
static void draw_title()
{
    char title[] = "Fanfare for the RP6502 Picocomputer";
    int16_t len = strlen(title);

    set_text_multiplier(1);
    set_text_colors(YELLOW, DARK_RED);
    set_cursor((CANVAS_W - CHAR_W*len)/2, 0);
    draw_string(title);
}

// ----------------------------------------------------------------------------
// ----------------------------------------------------------------------------
static void draw_reviews()
{
    set_text_multiplier(1);
    set_text_color(LIGHT_GRAY);
    set_cursor(0, CANVAS_H/2 - 16);
    draw_string("    'Smaller, lighter, faster than the Osborne-1!'\n");
    draw_string("                                             Byte\n\n");
    draw_string("    'Perfect for the 6502 professional on the go!'\n");
    draw_string("                                        Dr. Dobbs\n\n");
    draw_string("    'I wish I'd thought of it!'\n");
    draw_string("                      Steve #1\n\n");
    draw_string("    'I like the color!'\n");
    draw_string("              Steve #2\n\n\n");
    set_text_color(CYAN);
    draw_string("    'Thanks, Rumbledethumps, for a great project!'\n");
    draw_string("                                           tonyvr\n");
}

// ----------------------------------------------------------------------------
// ----------------------------------------------------------------------------
void move_asprite(uint8_t i, int16_t x, int16_t y)
{
    uint16_t ptr = asprite[i].config_ptr; // config address

    // update X
    RIA.addr0 = ptr + 12;
    RIA.step0 = ASPRITE_CONFIG_SIZE;
    RIA.addr1 = ptr + 13;
    RIA.step1 = ASPRITE_CONFIG_SIZE;

    RIA.rw0 = x & 0xff;
    RIA.rw1 = (x >> 8) & 0xff;

    // update Y
    RIA.addr0 = ptr + 14;
    RIA.addr1 = ptr + 15;

    RIA.rw0 = y & 0xff;
    RIA.rw1 = (y >> 8) & 0xff;

    asprite[i].x_pos = x;
    asprite[i].y_pos = y;
}

// ----------------------------------------------------------------------------
// ----------------------------------------------------------------------------
void init_asprite_config(uint8_t i)
{
    uint16_t ptr = asprite[i].config_ptr;

    xram0_struct_set(ptr, vga_mode4_asprite_t, transform[0], 1023);
    xram0_struct_set(ptr, vga_mode4_asprite_t, transform[1], 0);
    xram0_struct_set(ptr, vga_mode4_asprite_t, transform[2], 0);
    xram0_struct_set(ptr, vga_mode4_asprite_t, transform[3], 0);
    xram0_struct_set(ptr, vga_mode4_asprite_t, transform[4], 1023);
    xram0_struct_set(ptr, vga_mode4_asprite_t, transform[5], 0);

    xram0_struct_set(ptr, vga_mode4_asprite_t, x_pos_px, asprite[i].x_pos);
    xram0_struct_set(ptr, vga_mode4_asprite_t, y_pos_px, asprite[i].y_pos);
    xram0_struct_set(ptr, vga_mode4_asprite_t, xram_sprite_ptr, asprite[i].data_ptr);
    xram0_struct_set(ptr, vga_mode4_asprite_t, log_size, LOG_ASPRITE_SIZE);
    xram0_struct_set(ptr, vga_mode4_asprite_t, has_opacity_metadata, false);
}

// ----------------------------------------------------------------------------
// ----------------------------------------------------------------------------
void init_asprites()
{
    uint8_t i;    // asprite index
    uint16_t ptr; // config address

    // init breadboard
    i = BREADBOARD_INDEX;
    ptr = ASPRITE_CONFIGS + i*ASPRITE_CONFIG_SIZE;
    asprite[i].x_pos = -2*ASPRITE_SIZE;
    asprite[i].y_pos = -2*ASPRITE_SIZE;
    asprite[i].config_ptr = ptr;
    asprite[i].data_ptr = BREADBOARD_DATA;
    init_asprite_config(i);


    // init circuitboard
    i = CIRCUITBOARD_INDEX;
    ptr = ASPRITE_CONFIGS + i*ASPRITE_CONFIG_SIZE;
    asprite[i].x_pos = -2*ASPRITE_SIZE;
    asprite[i].y_pos = -2*ASPRITE_SIZE;
    asprite[i].config_ptr = ptr;
    asprite[i].data_ptr = CIRCUITBOARD_DATA;
    init_asprite_config(i);


    // init portable
    i = PORTABLE_INDEX;
    ptr = ASPRITE_CONFIGS + i*ASPRITE_CONFIG_SIZE;
    asprite[i].x_pos = -2*ASPRITE_SIZE;
    asprite[i].y_pos = -2*ASPRITE_SIZE;
    asprite[i].config_ptr = ptr;
    asprite[i].data_ptr = PORTABLE_DATA;
    init_asprite_config(i);

    // plane=1, affine sprite mode
    xreg_vga_mode(4, 1, ASPRITE_CONFIGS, TOTAL_ASPRITES, 1);
}

// ----------------------------------------------------------------------------
// ----------------------------------------------------------------------------
void main(void)
{
    uint8_t v = RIA.vsync;
    bool showing_breadboard = false;
    bool showing_circuitboard = false;
    bool showing_portable = false;
    bool showing_reviews = false;
    int16_t breadboard_scale = 1023;
    int16_t circuitboard_scale = 1023;
    int16_t portable_scale = 1023;

    asprite_timer = 0;

    _randomize();

    //printf("Hello, from Fanfare!\n");

    // initialize the graphics
    init_bitmap_graphics(0xFF00, 0x0000, 1, 1, CANVAS_W, CANVAS_H, 4); // plane=1, canvas=1, w=320, h=240, bpp4
    erase_canvas();
    draw_title();
    init_asprites();

    // initialize the music
    ezpsg_init(0xFF20);
    ezpsg_play_song(song);

    // vsync loop
    while (true)
    {
        if (v == RIA.vsync) {
            continue; // wait until vsync is incremented
        } else {
            v = RIA.vsync; // new value for v
            asprite_timer++;
        }

        if (!showing_breadboard && asprite_timer > breadboard_asprite_timer_threshold) {
            move_asprite(BREADBOARD_INDEX, CANVAS_W/2 - 4*ASPRITE_SIZE/2, CANVAS_H/9);
            if (asprite_timer > breadboard_asprite_timer_threshold + 100) {
                showing_breadboard = true;
            } else {
                if (breadboard_scale-8 > 255) {
                    breadboard_scale -= 8;
                } else {
                    breadboard_scale = 255;
                }
                xram0_struct_set(asprite[BREADBOARD_INDEX].config_ptr, vga_mode4_asprite_t,
                                 transform[0], breadboard_scale);
                xram0_struct_set(asprite[BREADBOARD_INDEX].config_ptr, vga_mode4_asprite_t,
                                 transform[4], breadboard_scale);
            }
        }
        if (!showing_circuitboard && asprite_timer > circuitboard_asprite_timer_threshold) {
            move_asprite(CIRCUITBOARD_INDEX, CANVAS_W/2 - ASPRITE_SIZE/2, CANVAS_H/9);
            if (asprite_timer > circuitboard_asprite_timer_threshold + 100) {
                showing_circuitboard = true;
            } else {
                if (circuitboard_scale-8 > 255) {
                    circuitboard_scale -= 8;
                } else {
                    circuitboard_scale = 255;
                }
                xram0_struct_set(asprite[CIRCUITBOARD_INDEX].config_ptr, vga_mode4_asprite_t,
                                 transform[0], circuitboard_scale);
                xram0_struct_set(asprite[CIRCUITBOARD_INDEX].config_ptr, vga_mode4_asprite_t,
                                 transform[4], circuitboard_scale);
            }
        }
        if (!showing_portable && asprite_timer > portable_asprite_timer_threshold) {
            move_asprite(PORTABLE_INDEX, CANVAS_W/2 + ASPRITE_SIZE, CANVAS_H/9);
            if (asprite_timer > portable_asprite_timer_threshold + 100) {
                showing_portable = true;
            } else {
                if (portable_scale-8 > 255) {
                    portable_scale -= 8;
                } else {
                    portable_scale = 255;
                }
                xram0_struct_set(asprite[PORTABLE_INDEX].config_ptr, vga_mode4_asprite_t,
                                 transform[0], portable_scale);
                xram0_struct_set(asprite[PORTABLE_INDEX].config_ptr, vga_mode4_asprite_t,
                                 transform[4], portable_scale);
            }
        }
        if (!showing_reviews && asprite_timer > reviews_asprite_timer_threshold) {
            draw_reviews();
            showing_reviews = true;
        }

        // play more music
        ezpsg_tick(12);
        if (!ezpsg_playing()) {
            break;
        }
    }
    //exit
    //printf("Goodbye!\n");
}
