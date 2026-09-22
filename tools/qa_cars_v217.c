#define R3_HOST
#include "../src/render3d.c"
#include <assert.h>
#include <stdio.h>

int main(void){
    R3Scene s={0};s.x=1000;s.z=1000;s.yaw=PI*.5f;s.cameraDistance=50;s.eyeHeight=34;
    s.carCount=24;
    for(int i=0;i<s.carCount;i++)s.cars[i]=(R3Car){930+(i%6)*16,955+(i/6)*18,0,0,i%6,0,i%12};
    r3_draw(NULL,&s);
    for(int m=0;m<MAT_COUNT;m++)if(r3_used(m)>4000)fprintf(stderr,"material %d vertices %d\n",m,r3_used(m));
    fprintf(stderr,"overflow %d\n",r3_overflow());
    assert(!r3_overflow());
    assert(r3_used(CAR_SIDE)>0&&r3_used(CAR_PAINT)>0&&r3_used(GLASS)>0);
    printf("PASS: 24 cars render without material overflow; %d side, %d paint, %d glass vertices.\n",
           r3_used(CAR_SIDE),r3_used(CAR_PAINT),r3_used(GLASS));
}
