/* Character combat, in projected world units. No allocations or save changes. */
typedef struct {float health,flee,fall,hit,yaw,phase;} CombatPed;
static struct {
 int aiming,target,lastWeapon,combo;float pitch,cooldown,recoil,punch,alert;
 float jump,velocity,vault,vaultX,vaultY,startX,startY;
 float shotX,shotY,shotHeight,shotTime;
 CombatPed ped[42];
} combat;
static volatile unsigned shotEvent;
static volatile int shotKind;
static void combat_reset(void){
 memset(&combat,0,sizeof(combat));combat.target=-1;
 for(int i=0;i<42;i++){combat.ped[i].health=100;combat.ped[i].phase=(float)i;}
}

static int combat_line(float x,float y,float xx,float yy){
 float a,b,c,d;physics_project(x,y,&a,&b);physics_project(xx,yy,&c,&d);
 int steps=(int)(hypotf(c-a,d-b)/2)+1;
 for(int j=1;j<steps;j++){float u,v;physics_unproject(a+(c-a)*j/steps,b+(d-b)*j/steps,&u,&v);if(solid(u,v))return 0;}
 return 1;
}
static void combat_damage(int i,float damage){
 CombatPed *p=&combat.ped[i];if(p->health<=0)return;
 p->health=fmaxf(0,p->health-damage);p->hit=.32f;p->flee=18;p->fall=0;
 combat.alert=12;g.escape=0;g.heat=clampf(g.heat+(p->health<=0?1.f:.35f),0,5);
 if(p->health<=0&&combat.target==i)combat.target=-1;
}
static void combat_camera_ray(float *origin,float *slope){
 float x,z,u,v;physics_project(g.x,g.y,&x,&z);
 float d=clampf(g.cameraDistance>0?g.cameraDistance:38,10,38);
 physics_unproject(x-cosf(g.viewYaw)*d-sinf(g.viewYaw)*5,z-sinf(g.viewYaw)*d+cosf(g.viewYaw)*5,&u,&v);
 float base=geo_height(g.x,g.y)+g.lift+combat.jump;
 float eye=fmaxf(base+14,geo_height(u,v)+2);
 *slope=(base+10+tanf(combat.pitch)*60-eye)/(60+d);*origin=eye+*slope*d;
}
static void combat_jump(void){
 if(combat.jump>0||combat.vault>0||g.inMetro||g.car>=0)return;
 float x,z;physics_project(g.x,g.y,&x,&z);
 float a=proj_heading(g.x,g.y,g.a),nx,ny;
 physics_unproject(x+cosf(a)*7,z+sinf(a)*7,&nx,&ny);
 /* Only known, reachable walls can be vaulted. Never teleport through a house,
    Metro pillar, cable tower, vehicle or the river boundary. */
 int parapet=nx>=1392&&nx<=1464;
 if(combat_wall_at(nx,ny,0)||(parapet&&cm_obstacle(nx,ny))){
  for(int k=12;k<=26;k+=2){
   physics_unproject(x+cosf(a)*k,z+sinf(a)*k,&nx,&ny);
   if(foot_free(nx,ny)&&fabsf(geo_height(nx,ny)-geo_height(g.x,g.y))<4){
    int clear=1;
    for(int j=1;j<k;j++){float u,v;physics_unproject(x+cosf(a)*j,z+sinf(a)*j,&u,&v);if(cm_solid(u,v)||fabsf(u-CM_METRO_X)<5)clear=0;}
    if(clear){combat.vault=.01f;combat.startX=g.x;combat.startY=g.y;combat.vaultX=nx;combat.vaultY=ny;return;}
   }
  }
 }
 combat.velocity=28;combat.jump=.001f;
}
static void combat_tick(float ax,float ay,float dt){
 if(g.car>=0||g.inMetro)combat.jump=combat.velocity=combat.vault=0;
 combat.cooldown=fmaxf(0,combat.cooldown-dt);combat.recoil=fmaxf(0,combat.recoil-dt);
 combat.punch=fmaxf(0,combat.punch-dt);combat.alert=fmaxf(0,combat.alert-dt);combat.shotTime=fmaxf(0,combat.shotTime-dt);
 if(combat.alert>0)g.escape=0;
 if(combat.vault>0){
  combat.vault+=dt/.95f;float t=clampf(combat.vault,0,1),ease=t*t*(3-2*t);
  float sx,sz,ex,ez;physics_project(combat.startX,combat.startY,&sx,&sz);physics_project(combat.vaultX,combat.vaultY,&ex,&ez);
  physics_unproject(sx+(ex-sx)*ease,sz+(ez-sz)*ease,&g.x,&g.y);
  combat.jump=sinf(PI*t)*22;
  if(t>=1){if(!foot_free(g.x,g.y)){g.x=combat.startX;g.y=combat.startY;}combat.vault=0;combat.jump=0;combat.velocity=0;}
 }else if(combat.jump>0){combat.velocity-=50*dt;combat.jump+=combat.velocity*dt;if(combat.jump<=0){combat.jump=0;combat.velocity=0;}}
 int was=combat.aiming;
 combat.aiming=g.car<0&&!g.inMetro&&!g.weaponWheel&&held(B_R);
 if(!combat.aiming){combat.target=-1;combat.pitch=0;if(was){g.stickActive=0;g.stickFX=g.stickFY=0;g.frameYaw=g.viewYaw;}return;}
 if(!was){combat.pitch=0;g.walking=0;g.footSpeed=0;g.stickActive=0;}
 float px,pz;physics_project(g.x,g.y,&px,&pz);
 float sx=ax-g.calX,sy=ay-g.calY;if(hypotf(sx,sy)<.20f)sx=sy=0;
 g.viewYaw+=sx*dt*1.7f;combat.pitch=clampf(combat.pitch-sy*dt, -.65f,.65f);
 int selected=-1;float score=1e9f;
 float range=g.weapon==0?28:g.weapon==7?36:190;
 for(int i=0;i<42;i++)if(combat.ped[i].health>0){
  float x,z;physics_project(g.peds[i].x,g.peds[i].y,&x,&z);float d=hypotf(x-px,z-pz);
  float angle=fabsf(angle_delta(atan2f(z-pz,x-px),g.viewYaw));
  float cone=i==combat.target?.72f:.43f;
  if(d<range&&angle<cone&&combat_line(g.x,g.y,g.peds[i].x,g.peds[i].y)){
   float s=angle*130+d*.25f-(i==combat.target?15:0);if(s<score){score=s;selected=i;}
  }
 }
 combat.target=selected;
 float aim=g.viewYaw;
 if(selected>=0){float x,z;physics_project(g.peds[selected].x,g.peds[selected].y,&x,&z);aim=atan2f(z-pz,x-px);
  combat.pitch=atan2f(geo_height(g.peds[selected].x,g.peds[selected].y)+9-(geo_height(g.x,g.y)+g.lift+10+combat.jump),hypotf(x-px,z-pz));}
 g.bodyYaw=aim;g.a=logical_heading_from_projected(g.x,g.y,aim);g.walking=0;
 if(!held(B_CIRCLE)||held(B_SQUARE)||combat.cooldown>0)return;
 int melee=g.weapon==0||g.weapon==7;
 static const float intervals[8]={.42f,.28f,.45f,.11f,.14f,.85f,.65f,.6f};
 static const float damage[8]={18,26,38,16,30,65,75,38};
 combat.cooldown=intervals[g.weapon];combat.recoil=.16f;combat.combo^=1;
 if(melee)combat.punch=.32f;
 shotKind=melee?0:g.weapon;shotEvent++;
 float reach=melee?(g.weapon==7?18:12):g.weapon==5?110:300;
 if(selected<0&&!melee){px-=sinf(aim)*5;pz+=cosf(aim)*5;}
 int victim=-1;float nearest=reach;
 float origin=geo_height(g.x,g.y)+g.lift+combat.jump+10;
 float slope=tanf(combat.pitch);if(selected<0&&!melee)combat_camera_ray(&origin,&slope);
 if(!melee){
  float carX[64],carZ[64],carA[64];
  for(int i=0;i<CAR_COUNT;i++){physics_project(g.cars[i].x,g.cars[i].y,&carX[i],&carZ[i]);carA[i]=physics_heading(&g.cars[i]);}
  for(float d=2;d<reach;d+=2){
   float x,z;physics_unproject(px+cosf(aim)*d,pz+sinf(aim)*d,&x,&z);float h=origin+slope*d,ground=geo_height(x,z);
   int blocked=world_solid(x,z)||(combat_wall_at(x,z,0)&&h<ground+18.7f)||h<ground+.3f;
   for(int i=0;i<CAR_COUNT&&!blocked;i++){
    float dx=px+cosf(aim)*d-carX[i],dz=pz+sinf(aim)*d-carZ[i];if(fabsf(dx)>25||fabsf(dz)>25)continue;
    if(fabsf(dx*cosf(carA[i])+dz*sinf(carA[i]))<18&&fabsf(-dx*sinf(carA[i])+dz*cosf(carA[i]))<9&&h<geo_height(g.cars[i].x,g.cars[i].y)+14)blocked=1;
   }
   if(blocked){nearest=d;break;}
  }
 }
 for(int i=0;i<42;i++)if(combat.ped[i].health>0){
  float x,z;physics_project(g.peds[i].x,g.peds[i].y,&x,&z);float dx=x-px,dz=z-pz;
  float along=dx*cosf(aim)+dz*sinf(aim),side=fabsf(-dx*sinf(aim)+dz*cosf(aim));
  float y=origin+slope*along;
  float floor=geo_height(g.peds[i].x,g.peds[i].y);
  if(along>0&&along<nearest&&side<(melee?5:3)&&y>=floor+1&&y<=floor+16&&combat_line(g.x,g.y,g.peds[i].x,g.peds[i].y)){victim=i;nearest=along;}
 }
 if(!melee){physics_unproject(px+cosf(aim)*nearest,pz+sinf(aim)*nearest,&combat.shotX,&combat.shotY);combat.shotHeight=origin+slope*nearest;combat.shotTime=.09f;}
 if(victim>=0)combat_damage(victim,damage[g.weapon]);
 else if(!melee){combat.alert=8;g.heat=clampf(g.heat+.08f,0,5);}
}
static void combat_peds_tick(float dt){
 for(int i=0;i<42;i++){
  Ped *p=&g.peds[i];CombatPed *c=&combat.ped[i];c->hit=fmaxf(0,c->hit-dt);
  if(c->health<=0){c->fall=fminf(1,c->fall+dt*1.6f);continue;}
  if(c->flee>0){
   c->flee-=dt;float px,pz,x,z;physics_project(g.x,g.y,&px,&pz);physics_project(p->x,p->y,&x,&z);
   float a=atan2f(z-pz,x-px),nx,ny;
   for(int k=0;k<7;k++){float off=k==0?0:((k+1)/2)*.55f*(k&1?1:-1);
    physics_unproject(x+cosf(a+off)*75*dt,z+sinf(a+off)*75*dt,&nx,&ny);
    if(foot_free(nx,ny)){p->x=nx;p->y=ny;c->yaw=logical_heading_from_projected(nx,ny,a+off);break;}
   }
   c->phase+=dt*12;
  }else{p->y+=p->v*dt;int local=(int)p->y%320;if(local<91||local>283)p->v=-p->v;c->yaw=p->v>0?PI*.5f:-PI*.5f;c->phase+=dt*6;}
 }
}
static void combat_hud(void){
 if(!combat.aiming||g.screen!=WORLD||g.weaponWheel)return;
 float x=240,y=136;int valid=1;
#ifdef NARCADE_3D
 if(combat.target>=0)valid=r3_target_screen(g.peds[combat.target].x,g.peds[combat.target].y,11,&x,&y);
#endif
 if(!valid)return;
 int xx=(int)x,yy=(int)y;uint32_t color=combat.target>=0?CORAL:WHITE;
 line(xx-11,yy,xx-5,yy,color);line(xx+5,yy,xx+11,yy,color);line(xx,yy-11,xx,yy-5,color);line(xx,yy+5,xx,yy+11,color);circle(xx,yy,1,color);
 if(combat.target>=0){rect(xx-16,yy+17,32,3,INK);rect(xx-16,yy+17,(int)(combat.ped[combat.target].health*.32f),3,CORAL);}
 text(152,250,g.weapon==0||g.weapon==7?"O golpear   [] interactuar":"O disparar   [] interactuar",WHITE,1);
}
