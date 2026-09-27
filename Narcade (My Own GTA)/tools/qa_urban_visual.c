#define R3_HOST
#include "../src/render3d.c"
#include <stdio.h>
#include <math.h>

static int valid_mesh(void){
    for(int m=0;m<MAT_COUNT;m++){
        if(used[m]<0||used[m]>MAX_VERTICES||used[m]%3)return 0;
        for(int i=0;i<used[m];i++)if(!isfinite(mesh[m][i].x)||!isfinite(mesh[m][i].y)||!isfinite(mesh[m][i].z))return 0;
    }
    return 1;
}
int main(void){
    R3Scene s={0};view=&s;clipEnabled=0;fixedGround=-1000000;
    for(int phase=0;phase<2;phase++){
        s.time=phase?160:100;day_update(s.time);s.x=0;s.z=0;
        for(int type=0;type<6;type++){
            memset(used,0,sizeof used);R3Car c={0};c.type=type;c.paint=type;c.angle=.17f*type;car(&c);
            if(!valid_mesh()||used[CAR_PAINT]<36||used[GLASS]<6){puts("car mesh invalid");return 1;}
        }
        memset(used,0,sizeof used);tree_shape(0,0,0,0);tree_shape(40,0,1,0);tree_shape(80,0,2,0);
        if(!valid_mesh()||used[LEAVES]<60){puts("tree mesh invalid");return 2;}
        geographic=1;s.x=CM_METRO_X;s.z=1040;s.metroZ=1040;
        for(int door=0;door<2;door++){
            memset(used,0,sizeof used);s.metroDoors=door;metro_train();
            if(!valid_mesh()||used[GLASS]<12||metroFlexible){puts("metro mesh invalid");return 3;}
        }
        geographic=0;
    }
    puts("PASS: six car profiles, three tree types and sliding metro geometry remain finite in day/night views.");
    return 0;
}
