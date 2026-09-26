/* A few park walls provide actual, solid ledges for the climb action. Stored
   in projected coordinates so collision and the rendered dimensions agree. */
static float climbWall[3][4];static int climbWallCount;
static int combat_wall_at(float x,float y,float radius){
 if(!climbWallCount)return 0;
 float px,pz;geo_project(x,y,&px,&pz);
 for(int i=0;i<climbWallCount;i++)if(fabsf(px-climbWall[i][2])<12+radius&&fabsf(pz-climbWall[i][3])<1.2f+radius)return 1;
 return 0;
}
static void combat_walls_init(void){
 climbWallCount=0;
 for(int z=0;z<7&&climbWallCount<3;z++)for(int x=0;x<8&&climbWallCount<3;x++){
  const CmParcel *p;int n=cm_parcels(x,z,&p);
  for(int j=0;j<n&&climbWallCount<3;j++)if(p[j].kind==1&&!(cm_flags(x,z)&CM_PLAZA)){
   float cx,cz,px,pz;cm_centroid(&p[j],&cx,&cz);geo_project(cx,cz,&px,&pz);
   pz-=30;geo_unproject(px,pz,&cx,&cz);
   if(cm_parcel_at(cx,cz)!=1||cm_obstacle(cx,cz))continue;
   float *w=climbWall[climbWallCount++];w[0]=cx;w[1]=cz;w[2]=px;w[3]=pz;
  }
 }
}
