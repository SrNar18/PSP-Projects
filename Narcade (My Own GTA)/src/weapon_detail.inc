static Point weapon_vertex(Point grip,float x,float y,float z,float wx,float wz,float angle){
 float pitch=view->aiming?view->aimPitch:0;
 return local(grip.x+x*cosf(pitch)-y*sinf(pitch),grip.y+x*sinf(pitch)+y*cosf(pitch),grip.z+z,wx,wz,angle);
}
static void weapon_tube(Point grip,float start,float end,float y,float radius,float x,float z,float angle){
 Point cap[8];
 for(int k=0;k<8;k++){
  float a=k*PI/4,b=(k+1)*PI/4;
  Point p=weapon_vertex(grip,start,y+cosf(a)*radius,sinf(a)*radius,x,z,angle);
  Point q=weapon_vertex(grip,end,y+cosf(a)*radius,sinf(a)*radius,x,z,angle);
  Point r=weapon_vertex(grip,end,y+cosf(b)*radius,sinf(b)*radius,x,z,angle);
  Point t=weapon_vertex(grip,start,y+cosf(b)*radius,sinf(b)*radius,x,z,angle);
  quad(WEAPON_METAL,p,q,r,t,shade(COLOR(153,164,174),.7f+.22f*cosf(a)),3,.25f);
  cap[k]=q;
 }
 Vertex face[8];for(int k=0;k<8;k++)face[k]=(Vertex){.5f,.5f,COLOR(12,16,20),cap[k].x,cap[k].y,cap[k].z};polygon(FLAT,face,8);
}
