
#include "doomgeneric.h"
#include "doomkeys.h"
#include "scancode.h"
#include <mako.h>
#include <stdbool.h>
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <ui.h>

static uint32_t start_time = 0;
static bool window_init = false;
static uint32_t *ui_buf = 0;

#define KEYQUEUE_SIZE 16

struct keyqueue_entry
{
  uint8_t key;
  bool pressed;
};

static unsigned keyqueue_read_idx = 0;
static unsigned keyqueue_write_idx = 0;
static struct keyqueue_entry keyqueue[KEYQUEUE_SIZE];

static uint8_t convert_to_doom_key(uint8_t key, uint8_t *out)
{
  switch (key) {
    case KB_SC_ESC:
      *out = KEY_ESCAPE;
      return 1;
    case KB_SC_ENTER:
      *out = KEY_ENTER;
      return 1;
    case KB_SC_QUOTE:
      *out = KEY_FIRE;
      return 1;
    case KB_SC_SPACE:
      *out = KEY_USE;
      return 1;
    case KB_SC_RSHIFT:
      *out = KEY_RSHIFT;
      return 1;
    case KB_SC_W:
    case KB_SC_UP:
      *out = KEY_UPARROW;
      return 1;
    case KB_SC_S:
    case KB_SC_DOWN:
      *out = KEY_DOWNARROW;
      return 1;
    case KB_SC_A:
    case KB_SC_LEFT:
      *out = KEY_LEFTARROW;
      return 1;
    case KB_SC_D:
    case KB_SC_RIGHT:
      *out = KEY_RIGHTARROW;
      return 1;
    case KB_SC_Y:
      *out = 'y';
      return 1;
    default:
      return 0;
  }
}

static void buffer_key(bool pressed, uint8_t code)
{
  uint8_t doom_key = 0;
  if (!convert_to_doom_key(code, &doom_key))
    return;
  keyqueue[keyqueue_write_idx].key = doom_key;
  keyqueue[keyqueue_write_idx].pressed = pressed;
  keyqueue_write_idx = (keyqueue_write_idx + 1) % KEYQUEUE_SIZE;
}

static void keyboard_handler(uint8_t code)
{
  if (code & 0x80) {
    code &= 0x7F;
    buffer_key(false, code);
  } else
    buffer_key(true, code);
}

void DG_Init()
{
  priority(2);

  ui_buf = malloc(DOOMGENERIC_RESX * DOOMGENERIC_RESY * sizeof(uint32_t));
  if (ui_buf == NULL)
    return;
  int32_t res = ui_acquire_window(ui_buf, "DOOM", DOOMGENERIC_RESX, DOOMGENERIC_RESY);
  if (res < 0)
    return;

  // wait until window creation event
  ui_event_t ev;
  res = ui_next_event(&ev);
  if (res < 0 || ev.type != UI_EVENT_WAKE)
    return;

  start_time = systime();
  window_init = true;
  uint32_t size = ev.width * ev.height * 4;
  memset(ui_buf, 0, size);
}

void DG_DrawFrame()
{
  if (!window_init)
    return;

  while (ui_poll_events()) {
    ui_event_t ev;
    ui_next_event(&ev);
    if (ev.type == UI_EVENT_WAKE)
      priority(2);
    if (ev.type == UI_EVENT_SLEEP)
      priority(1);
    if (ev.type == UI_EVENT_KEYBOARD)
      keyboard_handler(ev.code);
  }

  for (unsigned y = 0; y < DOOMGENERIC_RESY; ++y) {
    memcpy32(
      ui_buf + y * DOOMGENERIC_RESX, DG_ScreenBuffer + y * DOOMGENERIC_RESX, DOOMGENERIC_RESX);
  }

  ui_redraw_rect(0, 0, DOOMGENERIC_RESX, DOOMGENERIC_RESY);
}

int DG_GetKey(int *pressed, unsigned char *key)
{
  if (keyqueue_read_idx == keyqueue_write_idx)
    return 0;

  *pressed = keyqueue[keyqueue_read_idx].pressed;
  *key = keyqueue[keyqueue_read_idx].key;
  keyqueue_read_idx = (keyqueue_read_idx + 1) % KEYQUEUE_SIZE;
  return 1;
}

void DG_SleepMs(uint32_t ms)
{
  msleep(ms);
}

void DG_SetWindowTitle(const char *title) {}

uint32_t DG_GetTicksMs()
{
  return systime() - start_time;
}
