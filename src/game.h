#ifndef NARCADE_GAME_H
#define NARCADE_GAME_H
#include <stdint.h>
enum { B_SELECT=1, B_START=8, B_UP=16, B_RIGHT=32, B_DOWN=64, B_LEFT=128, B_L=256, B_R=512, B_TRI=4096, B_CIRCLE=8192, B_CROSS=16384, B_SQUARE=32768 };
void game_init(void);
void game_tick(unsigned buttons,float analogx,float analogy,float dt);
void game_draw(uint32_t *pixels,int stride);
void game_audio(short *stereo,unsigned frames);
int game_save(void);
void game_set_save_path(const char *path);
#endif
