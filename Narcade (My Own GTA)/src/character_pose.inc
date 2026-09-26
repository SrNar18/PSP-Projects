/* Two segment arm rig; coordinates match the supplied personaje-v3 mesh. */
static Point pose_hand(int bone){
 int right=bone==3;float sign=right?-1:1;
 Point h=point(.95f,12.8f,sign*3.95f);
 if(view->weapon==7){
  float t=1-view->punch/.32f,swing=view->punch>0?sinf(PI*geo_clamp(t,0,1))*1.3f:0;
  h=point(4.8f+swing*2,view->aiming?20:17,-3.2f);
  if(!right){h.x+=2*sinf(swing);h.y+=2*cosf(swing);}
 }else if(view->weapon>0&&view->weapon<7){
  if(right)h=point(view->aiming?4.8f:3.8f,view->aiming?20.2f:16.4f,-3.4f);
  else if(view->weapon>=3)h=point(view->aiming?8.2f:7.f,view->aiming?20.4f:17.f,-2.5f);
 }else if(view->aiming){
  h=point(4.4f,20.f,sign*3.9f);
  if(view->punch>0&&right==(view->combo==0)){
   float t=1-view->punch/.32f,thrust=sinf(PI*geo_clamp(t,0,1));h.x+=6*thrust;h.y-=thrust*.8f;
  }
 }
 if(view->climb>0)h=point(5.5f,25.4f,sign*3.8f);
 if(view->aiming&&view->weapon>0&&view->weapon<7){float f=h.x;h.y+=sinf(view->aimPitch)*f;h.x*=cosf(view->aimPitch);if(right)h.x-=view->recoil*4;}
 return h;
}
static Point pose_arm(Point p,int bone){
 float sign=bone==3?-1:1;
 Point shoulder=point(0,22,sign*4.8f),restElbow=point(0,16.5f,sign*4.3f),restHand=point(.95f,12.8f,sign*3.95f);
 Point hand=pose_hand(bone);
 float dx=hand.x-shoulder.x,dy=hand.y-shoulder.y,dz=hand.z-shoulder.z;
 float d=sqrtf(dx*dx+dy*dy+dz*dz),upper=6,lower=5;
 if(d>10.85f){float k=10.85f/d;hand=point(shoulder.x+dx*k,shoulder.y+dy*k,shoulder.z+dz*k);dx*=k;dy*=k;dz*=k;d=10.85f;}
 d=fmaxf(d,.01f);float along=(upper*upper-lower*lower+d*d)/(2*d),bend=sqrtf(fmaxf(0,upper*upper-along*along));
 /* Elbow bends down and slightly away from the rib cage. */
 Point axis=point(dx/d,dy/d,dz/d),perp=point(axis.x*axis.y,axis.y*axis.y-1,axis.z*axis.y);
 float n=sqrtf(perp.x*perp.x+perp.y*perp.y+perp.z*perp.z);if(n<.01f){perp=point(0,0,sign);n=1;}
 Point elbow=point(shoulder.x+axis.x*along+perp.x/n*bend,shoulder.y+axis.y*along+perp.y/n*bend,shoulder.z+axis.z*along+perp.z/n*bend);
 if(p.y>=22)return p;
 Point r0,r1,t0,t1;
 if(p.y>=16.5f){r0=shoulder;r1=restElbow;t0=shoulder;t1=elbow;}
 else{r0=restElbow;r1=restHand;t0=elbow;t1=hand;}
 float t=geo_clamp((r0.y-p.y)/(r0.y-r1.y),0,1);
 if(p.y<restHand.y)t=(r0.y-p.y)/(r0.y-r1.y);
 /* Rotate each segment's cross section, rather than stretch its diameter. */
 float cx=r0.x+(r1.x-r0.x)*t,cz=r0.z+(r1.z-r0.z)*t;
 Point direction=point(t1.x-t0.x,t1.y-t0.y,t1.z-t0.z);
 float len=sqrtf(direction.x*direction.x+direction.y*direction.y+direction.z*direction.z);
 float rx=-direction.y/len,ry=direction.x/len;
 return point(t0.x+(t1.x-t0.x)*t+(p.x-cx)*rx,t0.y+(t1.y-t0.y)*t+(p.x-cx)*ry,t0.z+(t1.z-t0.z)*t+(p.z-cz));
}
static Point player_surface(const PlayerVertex *a){
 Point p=point(a->x,a->y,a->z);
 /* A closed hair shell covers the scalp, including during head turns. */
 if(a->mat==HAIR){p.x*=1.10f;p.z*=1.10f;p.y=25.2f+(p.y-25.2f)*1.07f+.16f;}
 if((a->mat==SKIN||a->mat==FACE)&&a->bone==0&&p.y>27.1f){p.x*=.94f;p.z*=.94f;}
 /* Hide the skin underneath the sleeve; don't enlarge exposed forearms. */
 if(a->mat==SKIN&&a->bone>=3&&p.y>19){float c=(a->bone==3?-1:1)*4.8f;p.x*=.86f;p.z=c+(p.z-c)*.86f;}
 if(a->mat==SLEEVE){float c=(a->bone==3?-1:1)*4.8f;p.x*=1.06f;p.z=c+(p.z-c)*1.06f;}
 return p;
}
