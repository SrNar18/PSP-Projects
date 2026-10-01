#ifndef NARCADE_WORLD_GEO_H
#define NARCADE_WORLD_GEO_H
#include <math.h>
/* medellin_profile.h (tools/medellin_map.py) is the source of geo_rows below */
/* Save positions and mission routes retain their original logical coordinates.
   Rendering, navigation maps and altitude share this one geographic transform. */
static inline float geo_clamp(float x,float a,float b){return fminf(b,fmaxf(a,x));}
/* Hold the valley outline steady across each built block.  The transition is
   made inside the east/west street, so building facades stay straight while
   the avenues still follow Medellin's changing valley width.
   v2.52 (Claude): one outline per block row, taken from medellin_profile.h at the block centre and smoothed so
   neighbouring rows differ by at most 40 (left/right edge) and 30 (river). The raw profile jumped up to 507 units
   inside an 86-unit street: on the west edge (where a new game starts) the avenues and the metro viaduct made a
   sharp S, as if a piece of the map had been stretched. tools/qa_geo_v252.c checks the limit. */
static const float geo_rows[8][3]={
    {619.7f,1563.3f,2303.5f},{579.7f,1533.3f,2263.5f},{563.1f,1503.3f,2223.5f},{603.1f,1473.3f,2183.5f},
    {643.1f,1443.3f,2143.5f},{683.1f,1413.3f,2103.5f},{723.1f,1417.6f,2063.5f},{763.1f,1447.6f,2047.5f}};
static inline void geo_row(float z,float *left,float *river,float *right){
    float zz=geo_clamp(z,0.f,2239.999f),rowf=floorf(zz/320.f),local=zz-rowf*320.f;int row=(int)rowf,prev=row>0?row-1:0;
    float t=local>=86.f?1.f:geo_clamp(local/86.f,0.f,1.f);t=t*t*(3.f-2.f*t);
    if(z<0||row==0)t=1;
    *left=geo_rows[prev][0]+(geo_rows[row][0]-geo_rows[prev][0])*t;
    *river=geo_rows[prev][1]+(geo_rows[row][1]-geo_rows[prev][1])*t;
    *right=geo_rows[prev][2]+(geo_rows[row][2]-geo_rows[prev][2])*t;
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
/* v2.5: every east/west street and junction is a level terrace.
   Only north/south streets connect elevations. Four linear ramp spans
   approximate smoothstep, keeping render vertices and contact heights exact.
   Maximum rise is 28 over 351 projected units: under 7 degrees. */
static const float geo_levels[8]={86,110,96,68,42,22,36,50};
static inline float geo_next_z(float z){
    float row=floorf(z/320),v=z-row*320;
    return row*320+(v<85.999f?86:86+(floorf((v-86)/58.5f)+1)*58.5f);
}
static inline float geo_height(float x,float z){
    float zz=geo_clamp(z,0,2239.999f),row=floorf(zz/320),v=zz-row*320;int i=(int)row;
    float t=geo_clamp((v-86)/234,0,1)*4;int k=(int)fminf(t,3);float f=t-k;
    static const float ease[5]={0,.15625f,.5f,.84375f,1};
    float h=geo_levels[i]+(geo_levels[i+1]-geo_levels[i])*(ease[k]+(ease[k+1]-ease[k])*f);
    /* Scenic slopes are outside the drivable urban footprint. */
    float outside=fmaxf(0,fmaxf(-x,x-2560))/320;
    return h+geo_clamp(outside,0,2)*95;
}
static inline float geo_heading(float x,float z,float angle){
    float a,b,c,d;geo_project(x,z,&a,&b);geo_project(x+cosf(angle)*4,z+sinf(angle)*4,&c,&d);return atan2f(d-b,c-a);
}
#endif
