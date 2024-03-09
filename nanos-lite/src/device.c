#include "common.h"

#define NAME(key) \
  [_KEY_##key] = #key,

static const char *keyname[256] __attribute__((used)) = {
  [_KEY_NONE] = "NONE",
  _KEYS(NAME)
};

size_t events_read(void *buf, size_t len) {
  char str[20];
  bool down = false;
  int key = _read_key();
  if(key & 0x8000) {
    key ^= 0x8000;
    down = true;
  }
  if(key != _KEY_NONE) {
    sprintf(str, "%s %s\n", down ? "kd" : "ku", keyname[key]);
  }
  else {
    sprintf(str, "t %d\n", _uptime());
  }
  if(down && key == _KEY_F12){
    extern void switch_current_game();
    switch_current_game();
    Log("key down:_KEY_F12,switch current game");
  }
  if(strlen(str) <= len) {
    strncpy((char *)buf, str, strlen(str));
    return strlen(str);
  }
  Log("strlen(event)>len,return 0");
  return 0;
}

static char dispinfo[128] __attribute__((used));

void dispinfo_read(void *buf, off_t offset, size_t len) {
  strncpy(buf, dispinfo + offset, len);
}

extern void getScreen(int *p_width, int *p_height);
void fb_write(const void *buf, off_t offset, size_t len) {
  assert(offset % 4 == 0 && len % 4 == 0);
  int index,x1,y1,y2;
  int width = 0, height = 0;
  getScreen(&width, &height);

  index = offset / 4;
  y1 = index / width;
  x1 = index % width;

  index = (offset + len) / 4;
  y2 = index / width;

  assert(y2 >= y1);

  if(y2 == y1)
  {
    _draw_rect(buf, x1, y1, len / 4, 1);
    return;
  }

  int t_width = width - x1;

  if(y2 - y1 == 1)
  {
    _draw_rect(buf, x1, y1, t_width, 1);
    _draw_rect(buf + t_width * 4, 0, y2, len / 4 - t_width, 1);
    return;
  }

  _draw_rect(buf, x1, y1, t_width, 1);
  int t_y = y2 - y1 - 1;
  _draw_rect(buf + t_width * 4, 0, y1 + 1, width, t_width);
  _draw_rect(buf + t_width * 4 + t_y * t_width * 4, 0, y2, len / 4 - t_width - t_y * width, 1);

  /*for(int i = 0; i < len / 4; i++) {
    index = offset / 4 + i;
    y1 = index / width;
    x1 = index % width;
    _draw_rect(buf + i * 4, x1, y1, 1, 1);
  }*/
}

void init_device() {
  _ioe_init();

  // TODO: print the string to array `dispinfo` with the format
  // described in the Navy-apps convention
  int width = 0, height = 0;
  getScreen(&width, &height);
  sprintf(dispinfo, "WIDTH:%d\nHEIGHT:%d\n", width, height);
}
