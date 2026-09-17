#ifndef NARCADE_RENDER3D_H
#define NARCADE_RENDER3D_H
#include <stdint.h>

/* World X/Z use the same units as the original game X/Y. Height is Y. */
typedef struct { float x,z,angle,speed; int type,police; } R3Car;
typedef struct { float x,z,angle; int style; } R3Person;
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
    float eyeHeight; /* v2.6.2: altura de la camara sobre el jugador (zoom con SELECT) */ /* v2.6: altura peatonal (anden) y tren del Metro */
} R3Scene;
void r3_init(void);
int r3_overflow(void);int r3_used(int m); /* diagnostico: poligonos descartados por presupuesto y vertices usados por material */
void r3_draw(uint32_t *framebuffer, const R3Scene *scene);
void r3_overlay(uint32_t *framebuffer,const uint32_t *rgba);
void r3_shutdown(void);
#endif
