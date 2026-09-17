#ifndef NARCADE_WORLD_GEO_H
#define NARCADE_WORLD_GEO_H
#include <math.h>
#include "medellin_profile.h"
/* Save positions and mission routes retain their original logical coordinates.
   Rendering, navigation maps and altitude share this one geographic transform. */
static inline float geo_clamp(float x,float a,float b){return fminf(b,fmaxf(a,x));}
static inline void geo_row(float z,float *left,float *river,float *right){
    float v=geo_clamp(z/70,0,31.99999f);int i=(int)v;float t=v-i;
    *left=medellin_profile[i][0]*(1-t)+medellin_profile[i+1][0]*t;
    *river=medellin_profile[i][1]*(1-t)+medellin_profile[i+1][1]*t;
    *right=medellin_profile[i][2]*(1-t)+medellin_profile[i+1][2]*t;
}
static inline void geo_project(float x,float z,float *gx,float *gz){
    float l,r,c;geo_row(z,&l,&c,&r);
    *gx=x<1428?l+(c-l)*x/1428:c+(r-c)*(x-1428)/1132;
    *gz=z*1.5f;
}
static inline void geo_unproject(float gx,float gz,float *x,float *z){
    float l,r,c;*z=gz/1.5f;geo_row(*z,&l,&c,&r);
    *x=gx<c?(gx-l)*1428/fmaxf(c-l,1):1428+(gx-c)*1132/fmaxf(r-c,1);
}
static inline float geo_height_raw(float x,float z){
    float west=geo_clamp((1120-x)/1120,0,1.5f),east=geo_clamp((x-1530)/1030,0,1.5f);
    /* San Javier and western hills; Popular/Santo Domingo and eastern slopes.
       Elevations are game units, NOT a surveyed digital elevation model. */
    float north=geo_clamp((1050-z)/1050,0,1),south=geo_clamp((z-1200)/1040,0,1);
    return 4+z*.007f+west*west*(190+30*south)+east*east*(220+90*north+25*south);
}
static inline float geo_height(float x,float z){
    /* Match the same 80-unit triangular terrain lattice used by ground(). */
    float bx=floorf(x/80)*80,bz=floorf(z/80)*80,u=(x-bx)/80,v=(z-bz)/80;
    float a=geo_height_raw(bx,bz),b=geo_height_raw(bx+80,bz),c=geo_height_raw(bx+80,bz+80),d=geo_height_raw(bx,bz+80);
    return u>=v?a+(b-a)*u+(c-b)*v:a+(c-d)*u+(d-a)*v;
}
static inline float geo_heading(float x,float z,float angle){
    float a,b,c,d;geo_project(x,z,&a,&b);geo_project(x+cosf(angle)*4,z+sinf(angle)*4,&c,&d);return atan2f(d-b,c-a);
}
#endif
