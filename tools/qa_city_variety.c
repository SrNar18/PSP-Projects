#define R3_HOST
#include "../src/render3d.c"
#include <assert.h>
#include <stdio.h>

int main(void){
    int counts[CB_WORKSHOP+1]={0},seen[4]={0},maxVertices=0;
    for(int bz=0;bz<7;bz++)for(int bx=0;bx<8;bx++){
        counts[city_block_type(bx,bz)]++;
        for(int h=0;h<2;h++)for(int direction=0;direction<4;direction++){
            R3Scene s={0};s.x=bx*320+42;s.z=bz*320+42;
            s.yaw=direction*PI*.5f;s.cameraDistance=58;s.eyeHeight=40;
            s.target=-1;s.metroZ=1200;s.metroDir=1;
            s.time=(h?.79f:.28f)*DAY_SECONDS;
            r3_draw(NULL,&s);assert(!overflow);
            int total=0;for(int m=0;m<MAT_COUNT;m++)total+=used[m];
            if(total>maxVertices)maxVertices=total;
            for(int m=0;m<4;m++)if(used[RETAIL+m]>0)seen[m]++;
        }
    }
    for(int m=CB_RETAIL;m<=CB_WORKSHOP;m++)assert(counts[m]>0);
    for(int m=0;m<4;m++)assert(seen[m]>0);
    for(int i=1;i<5;i++){
        float x,z;cm_cable_node(i,&x,&z);
        assert(!cm_solid(x,z));assert(cm_obstacle(x,z));
    }
    assert(cm_solid(CM_CABLE_X0,CM_CABLE_Z0));
    assert(cm_solid(CM_CABLE_X1,CM_CABLE_Z1));
    for(int bz=0;bz<7;bz++)for(int bx=0;bx<8;bx++){
        unsigned f=cm_flags(bx,bz);float x=bx*320,z=bz*320;
        if(f&CM_SPLIT_X)for(int dx=-23;dx<=23;dx+=23)
            assert(cm_on_grid_road(x+192+dx,z+120)&&!cm_solid(x+192+dx,z+120));
        if(f&CM_SPLIT_Z)for(int dz=-23;dz<=23;dz+=23)
            assert(cm_on_grid_road(x+120,z+186+dz)&&!cm_solid(x+120,z+186+dz));
        if(f&CM_TUNNEL)for(int dx=-18;dx<=18;dx+=18)
            assert(cm_on_grid_road(x+359+dx,z+120)&&!cm_solid(x+359+dx,z+120));
    }
    printf("PASS: 448 neighborhood/day/night views, 4 new types/materials visible, no overflow; max %d vertices.\n",maxVertices);
    puts("PASS: wider side streets and tunnels, cable pylons clear of buildings and physically solid.");
}
