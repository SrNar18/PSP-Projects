/* Regress changing train tessellation/UVs at fixed city lattice boundaries. */
#define R3_HOST
#include "../src/render3d.c"
#include <stdio.h>
#include <stdint.h>
static uint64_t uv_hash(void){
    uint64_t h=1469598103934665603ull;
    for(int m=0;m<MAT_COUNT;m++){
        h=(h^(unsigned)used[m])*1099511628211ull;
        for(int i=0;i<used[m];i++){unsigned u,v;memcpy(&u,&mesh[m][i].u,4);memcpy(&v,&mesh[m][i].v,4);
            h=(h^u)*1099511628211ull;h=(h^v)*1099511628211ull;
            if(!isfinite(mesh[m][i].x)||!isfinite(mesh[m][i].y)||!isfinite(mesh[m][i].z))return 0;}
    }return h;
}
static int floor_corner(float x,float z){
 float gx,gz;geo_project(x,z,&gx,&gz);
 float y=geo_height(CM_METRO_X,z)+CM_PLAT_H+.2f+.15f+1.05f;
 for(int i=0;i<used[SIDEWALK];i++){
  Vertex *v=&mesh[SIDEWALK][i];
  if(fabsf(v->x-gx)<.002f&&fabsf(v->y-y)<.002f&&fabsf(v->z-gz)<.002f)return 1;
 }
 return 0;
}
int main(void){
    R3Scene s={0};view=&s;geographic=1;clipEnabled=0;s.x=CM_METRO_X;s.time=100;day_update(100);
    for(int doors=0;doors<2;doors++){
        uint64_t expected=0;s.metroDoors=doors;
        for(int frame=0;frame<240;frame++){
            memset(used,0,sizeof used);overflow=0;s.z=s.metroZ=270+frame*.75f;metro_train();uint64_t h=uv_hash();
            if(!h||overflow||metroFlexible){puts("FAIL: train mesh invalid");return 1;}
            /* Both car ends use the exact same geographic transform and
               centreline elevation as the stationary rails. */
            for(int c=0;c<2;c++)for(int end=-1;end<=1;end+=2){
                float zc=s.metroZ+(c-.5f)*(CM_TRAIN_CAR+6),zz=zc+end*CM_TRAIN_CAR*.5f;
                if(!floor_corner(CM_METRO_X-10,zz)||!floor_corner(CM_METRO_X+10,zz)){
                    printf("FAIL: floating train corner frame %d car %d end %d\n",frame,c,end);return 3;
                }
            }
            if(!frame)expected=h;else if(h!=expected){printf("FAIL: UV topology changes at frame %d\n",frame);return 2;}
        }
    }
    puts("PASS: 480 moving-train frames across terraces preserve UVs/topology with doors open and closed.");return 0;
}
