#define R3_HOST
#include "../src/render3d.c"
#include <stdio.h>
#include <stdlib.h>
int main(void){
    R3Scene s={0};view=&s;geographic=0;rigid=0;clipEnabled=0;s.time=100;day_update(100);
    for(int i=0;i<9;i++){
        memset(used,0,sizeof used);s.x=0;s.z=0;
        if(i<6){R3Car c={0};c.x=0;c.z=0;c.angle=.25f;c.type=i;c.paint=i+2;car(&c);
            eye=point(58,27,44);target=point(0,8,0);
        }else if(i==6){geographic=1;s.x=CM_METRO_X;s.z=1030;s.metroZ=1040;
            eye=geo_point(CM_METRO_X+80,46,1080);target=geo_point(CM_METRO_X,16,1040);metro_train();
        }else{geographic=0;s.x=0;s.z=0;tree_shape(0,0,i==7?0:1,0);
            eye=point(52,40,62);target=point(0,28,0);
        }
        char path[100];snprintf(path,sizeof path,"build/urban-visual-%d.bin",i);
        FILE *f=fopen(path,"wb");if(!f)return 1;
        fwrite(&eye,sizeof eye,1,f);fwrite(&target,sizeof target,1,f);
        for(int m=0;m<MAT_COUNT;m++){fwrite(&used[m],4,1,f);fwrite(mesh[m],sizeof(Vertex),used[m],f);}
        fclose(f);geographic=0;fixedGround=-1000000;
    }
    return 0;
}
