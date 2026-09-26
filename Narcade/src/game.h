#ifndef NARCADE_GAME_H
#define NARCADE_GAME_H
#include <stdint.h>
enum { B_SELECT=1, B_START=8, B_UP=16, B_RIGHT=32, B_DOWN=64, B_LEFT=128, B_L=256, B_R=512, B_TRI=4096, B_CIRCLE=8192, B_CROSS=16384, B_SQUARE=32768 };
void game_init(void);
void game_latch_cross(unsigned pressed);
void game_tick(unsigned buttons,float analogx,float analogy,float dt);
void game_draw(uint32_t *pixels,int stride);
void game_audio(short *stereo,unsigned frames);
int game_save(void);
void game_set_save_path(const char *path);
/* Guardado nativo PSP (dialogo de la Memory Stick). 1=guardar, 2=cargar, 3=guardar y salir al titulo. */
void game_set_native_savedata(int on);
void game_set_lowmem(int kb);void game_set_lowmem2(const char *msg);
void game_loading_screen(uint32_t *pixels,int stride);void game_world_reset(void);void game_player_pos(int *x,int *y);
int game_load_stage(uint32_t *pixels,int stride,int stage); /* v2.13.3: carga por etapas */
int game_take_request(void);
void game_request_result(int req,int ok);
int game_export_save(void *buf,int cap);
int game_import_save(const void *buf,int len);
void game_save_summary(char *title,int titlecap,char *detail,int detailcap);
void game_continue(void);
void game_set_profile(float ms);
#endif
