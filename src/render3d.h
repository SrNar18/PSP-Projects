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
} R3Scene;
void r3_init(void);
void r3_draw(uint32_t *framebuffer, const R3Scene *scene);
void r3_overlay(uint32_t *framebuffer,const uint32_t *rgba);
void r3_shutdown(void);
#endif
