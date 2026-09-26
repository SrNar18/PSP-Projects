#include "../src/game.c"
#define FIELD(o,f) printf("\"" #f "\":%u,",(unsigned)offsetof(__typeof__(o),f))
int main(void){
 printf("{\"g\":{");FIELD(g,screen);FIELD(g,x);FIELD(g,y);FIELD(g,weapon);FIELD(g,weaponWheel);FIELD(g,heat);FIELD(g,viewYaw);printf("\"size\":%u},\"combat\":{",(unsigned)sizeof(g));
 FIELD(combat,aiming);FIELD(combat,target);FIELD(combat,jump);FIELD(combat,recoil);FIELD(combat,punch);FIELD(combat,shotTime);printf("\"size\":%u}}\n",(unsigned)sizeof(combat));return 0;
}
