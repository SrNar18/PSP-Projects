/* Real surface details derived from the six body profiles. They attach to the
   actual sloping shell, with no copied door/window bitmap or new PSP texture. */
static void car_arch(float x,float z,float angle,const HullSec *body,int n,
                     float floor,float centre,float wheelRadius,int side,uint32_t paint){
    float radius=wheelRadius+1.05f;
    for(int k=0;k<5;k++){
        float a=PI*k/5,b=PI*(k+1)/5;
        float xa=centre+cosf(a)*radius,xb=centre+cosf(b)*radius;
        float ya=wheelRadius+sinf(a)*radius,yb=wheelRadius+sinf(b)*radius;
        /* A dark wheel-well shadow plus a narrow body-colored rolled lip. */
        Point p=car_skin(x,z,angle,body,n,floor,xa,ya,side,.53f);
        Point q=car_skin(x,z,angle,body,n,floor,xb,yb,side,.53f);
        Point r=car_skin(x,z,angle,body,n,floor,xb,yb+.64f,side,.57f);
        Point s=car_skin(x,z,angle,body,n,floor,xa,ya+.64f,side,.57f);
        quad(METAL,p,q,r,s,day_scale(COLOR(37,43,46)),1,1);
        Point t=car_skin(x,z,angle,body,n,floor,xb,yb+.78f,side,.61f);
        Point u=car_skin(x,z,angle,body,n,floor,xa,ya+.78f,side,.61f);
        quad(CAR_PAINT,s,r,t,u,shade(paint,.78f),1,1);
    }
}
static void car_windscreen_trim(float x,float z,float angle,
                                const HullSec *body,int n,const unsigned char *mats,uint32_t paint){
    /* Both windscreens are steeply sloped glass top panels. Two wipers, thin
       rubber surround and a narrow metallic roof drip rail read at 480x272. */
    for(int i=0;i<n-1;i++){
        if(mats[i]!=200+GLASS)continue;
        const HullSec *a=&body[i],*b=&body[i+1];
        float t=i<n/2?.22f:.78f;
        float xx=a->x+(b->x-a->x)*t,yy=a->y+(b->y-a->y)*t+.14f;
        float width=(a->w+(b->w-a->w)*t)*.72f;
        for(int side=-1;side<=1;side+=2){
            float z0=side*.25f,z1=side*(width-.6f);
            Point p=local(xx,yy,z0,x,z,angle);
            Point q=local(xx+.12f,yy+.02f,z0,x,z,angle);
            Point r=local(xx+.12f,yy+.02f,z1,x,z,angle);
            Point s=local(xx,yy,z1,x,z,angle);
            quad(METAL,p,q,r,s,day_scale(COLOR(39,45,48)),1,1);
        }
    }
    for(int side=-1;side<=1;side+=2){
        for(int i=0;i<n-1;i++){
            const HullSec *a=&body[i],*b=&body[i+1];
            if(a->y<13||b->y<13)continue;
            float za=side*(a->w*.86f+.26f),zb=side*(b->w*.86f+.26f);
            Point p=local(a->x,a->y-.2f,za,x,z,angle);
            Point q=local(b->x,b->y-.2f,zb,x,z,angle);
            Point r=local(b->x,b->y+.18f,zb,x,z,angle);
            Point s=local(a->x,a->y+.18f,za,x,z,angle);
            quad(METAL,p,q,r,s,shade(paint,1.15f),1,1);
        }
    }
}
static void car_lens_detail(float x,float z,float angle,
                            const HullSec *body,int n,float floor,int side){
    /* Separate amber indicator, white running strip and red rear reflector. */
    float nose=body[n-1].x,tail=body[0].x;
    Point p=car_skin(x,z,angle,body,n,floor,nose-1.8f,floor+3.9f,side,.52f);
    box(p.x,p.z,p.y,1.4f,1.2f,.65f,angle,METAL,METAL,COLOR(251,168,65));
    p=car_skin(x,z,angle,body,n,floor,nose-.7f,floor+5.35f,side,.54f);
    box(p.x,p.z,p.y,1.7f,1.5f,.35f,angle,GLASS,GLASS,COLOR(221,235,231));
    p=car_skin(x,z,angle,body,n,floor,tail+1.1f,floor+4.3f,side,.52f);
    box(p.x,p.z,p.y,1.4f,1.35f,.50f,angle,METAL,METAL,COLOR(195,49,42));
}
