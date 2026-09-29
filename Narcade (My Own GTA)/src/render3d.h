#ifndef NARCADE_RENDER3D_H
#define NARCADE_RENDER3D_H
#include <stdint.h>

/* World X/Z use the same units as the original game X/Y. Height is Y. */
typedef struct { float x,z,angle,speed; int type,police,paint; } R3Car;
typedef struct { float x,z,angle; int style; float health,flee,fall,hit,phase; } R3Person;
typedef struct {
    float x,z,angle,yaw,time,cameraDistance,gaitPhase,motion;
    int driving,moving,target,carCount,personCount;
    float targetX,targetZ;
    uint32_t collected;
    float hubs[30][2];
    R3Car cars[64];
    R3Person people[42];
    float lift,metroZ;int metroDir,inMetro;
    float metroDoors; /* 0 closed, 1 open; driven by the station dwell timer */
    float camBase,camClear; /* v2.30: altura base y suelo minimo de camara ya suavizados por el juego (0 = calcular aqui) */
    float eyeHeight; /* v2.6.2: altura de la camara sobre el jugador (zoom con SELECT) */ /* v2.6: altura peatonal (anden) y tren del Metro */
    float shotX,shotZ,shotHeight,shotTime;
    int wallCount;float walls[3][3];
    int aiming,combo;float aimPitch,cameraPitch,recoil,punch,jump,climb;
    int weapon; /* 0 fists; 1..7 handheld models */
} R3Scene;
int r3_target_screen(float x,float z,float height,float *sx,float *sy);
void r3_init(void);
void r3_set_draw_buffer(uint32_t *fb);
void r3_gu_buffers(uint32_t *draw,uint32_t *disp);void r3_gu_display(int on);void r3_gu_idle(void);void r3_gu_swap(void); /* v2.9.1: dialogos del sistema */ /* v2.9: buffer de dibujo del GE para los dialogos del sistema */
void r3_trace(void (*fn)(const char*),int frames);void r3_phase_hook(void (*fn)(const char*)); /* v2.13.4: trazas de diagnostico */
int r3_overflow(void);int r3_used(int m); /* diagnostico: poligonos descartados por presupuesto y vertices usados por material */
void r3_draw(uint32_t *framebuffer, const R3Scene *scene);
void r3_loading_image(uint32_t *fb,const uint16_t *img,float zoom,int light,int alpha); /* v2.44 */
int r3_prewarm(const R3Scene *scene); /* v2.37: llena la cache de ciudad sin dibujar; 0 = listo */
void r3_overlay(uint32_t *framebuffer,const uint32_t *rgba);
void r3_overlay_bands(uint32_t *fb,const uint32_t *rgba,const unsigned char *bands); /* v2.41 */
void r3_set_brightness(int level); /* -5..5; display adjustment, no terrain rebuild */
void r3_set_distance_fog(int enabled);
void r3_shutdown(void);
#endif
