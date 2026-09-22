#include <assert.h>
#include <math.h>
#include <stdio.h>
#include "../src/world_geo.h"
#include "../src/citymap.h"

int main(void){
    int solid=0,roads=0,varied=0;
    for(int bz=0;bz<7;bz++)for(int bx=0;bx<8;bx++){
        const CmParcel *parcels;int n=cm_parcels(bx,bz,&parcels);
        for(int i=0;i<n;i++){
            const CmParcel *p=&parcels[i];
            if(p->kind!=0)continue;
            float cx,cz;cm_centroid(p,&cx,&cz);
            assert(cm_solid(cx,cz));solid++;
            if(cm_is_rect(p)){
                float ax,az,bx2,bz2;
                geo_project(p->x[0],p->z[0],&ax,&az);
                geo_project(p->x[3],p->z[3],&bx2,&bz2);
                if((int)(p->z[0]/320)==(int)(p->z[3]/320))
                    assert(fabsf(ax-bx2)<.01f); /* straight facade */
                if(fabsf(p->x[1]-p->x[0]-194)>2) varied++;
            }
        }
    }
    for(int z=12;z<2200;z+=20)for(int x=12;x<2550;x+=20){
        float gx,gz,ux,uz;geo_project(x,z,&gx,&gz);geo_unproject(gx,gz,&ux,&uz);
        assert(fabsf(x-ux)<.02f&&fabsf(z-uz)<.02f);
        if(cm_on_road(x,z)&&!cm_solid(x,z))roads++;
    }
    assert(solid>50&&roads>1000&&varied>20);
    for(int bz=0;bz<7;bz++)for(int bx=0;bx<8;bx++)if(cm_roundabout(bx,bz)){
        float cx=bx*320+42,cz=bz*320+42;
        assert(cm_on_road(cx,cz)&&cm_obstacle(cx,cz));
        assert(!cm_obstacle(cx+24,cz));
    }
    for(int row=0;row<7;row++)if(cm_footbridge_z(row)>0){
        float z=cm_footbridge_z(row);
        assert(!cm_obstacle(1428,z));
        assert(cm_obstacle(1428,z+14.5f));
    }
    puts("PASS: straight urban facades, varied parcels, reversible valley projection, drivable roundabouts.");
}
