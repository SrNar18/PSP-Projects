/* Animation only: no input, steering, collision or stamina changes.
   Joint targets and all trig/square roots are prepared once for each frame. */
static float poseWave,poseRun,poseMotion,poseBreath,poseBob,poseSway;
static float footStep[2],footLift[2],footRoll[2];
static float elbowSin,elbowCos;
typedef struct {float x,y,c,s;} GaitJoint;
static GaitJoint gaitHip[2],gaitKnee[2],gaitAnkle[2];
static float gait_smooth(float t){return t*t*(3-2*t);}
static void pose_prepare(void){
    poseMotion=geo_clamp(view->motion,0,2.1f);
    float moving=geo_clamp(poseMotion,0,1);
    poseRun=gait_smooth(geo_clamp((poseMotion-1)/1.08f,0,1));
    poseWave=sinf(view->gaitPhase);
    elbowSin=sinf(.15f*moving+poseRun*.95f);
    elbowCos=cosf(.15f*moving+poseRun*.95f);
    poseBreath=sinf(view->time*2.2f)*.065f*(1-moving);
    /* Pelvis rises at mid-stance; neither planted foot receives this bob. */
    poseBob=((.12f+.15f*poseRun)*cosf(2*view->gaitPhase)-(.22f+.18f*poseRun))*moving;
    poseSway=sinf(view->gaitPhase)*.16f*moving*(1-.65f*poseRun);
    for(int i=0;i<2;i++){
        float phase=fmodf(view->gaitPhase/(2*PI)+i*.5f,1);
        if(phase<0)phase+=1;
        float support=.62f-.14f*poseRun,stride=(3.9f+.6f*poseRun)*moving;
        if(phase<support){
            float t=phase/support;
            /* Almost constant backward velocity while the foot supports weight. */
            footStep[i]=stride*(1-2*t);footLift[i]=0;
            footRoll[i]=moving*(.10f*(1-gait_smooth(geo_clamp(t*5,0,1)))
                -.19f*gait_smooth(geo_clamp((t-.75f)*4,0,1)));
        }else{
            float t=(phase-support)/(1-support),arc=sinf(PI*t);
            footStep[i]=stride*(-1+2*gait_smooth(t));
            footLift[i]=arc*arc*(1.7f+3.2f*poseRun)*moving;
            footRoll[i]=(-.19f*(1-t)+.10f*t-.08f*arc)*moving;
        }
        GaitJoint h={0,14.8f+poseBob,1,0},a={footStep[i],.7f+footLift[i],1,0};
        float dy=a.y-h.y;
        /* Keep both 7.4-unit bones within reach, even at sprint heel strike. */
        float reach=sqrtf(fmaxf(0,14.65f*14.65f-dy*dy));
        footStep[i]=geo_clamp(footStep[i],-reach,reach);a.x=footStep[i];
        float dx=a.x-h.x,dist=sqrtf(dx*dx+dy*dy);
        float along=dist*.5f,bend=sqrtf(fmaxf(0,7.4f*7.4f-along*along));
        float inv=1/fmaxf(dist,.01f);
        GaitJoint k={h.x+dx*.5f-dy*inv*bend,h.y+dy*.5f+dx*inv*bend,1,0};
        /* Blend out the bent rig at rest, preserving the original mesh silhouette. */
        k.x*=moving;k.y=7.4f+(k.y-7.4f)*moving;
        float tx=k.x-h.x,ty=k.y-h.y,len=sqrtf(tx*tx+ty*ty);
        h.c=-ty/fmaxf(len,.01f);h.s=tx/fmaxf(len,.01f);
        tx=a.x-k.x;ty=a.y-k.y;len=sqrtf(tx*tx+ty*ty);
        k.c=-ty/fmaxf(len,.01f);k.s=tx/fmaxf(len,.01f);
        gaitHip[i]=h;gaitKnee[i]=k;gaitAnkle[i]=a;
    }
}
static Point gait_segment(Point p,GaitJoint root,GaitJoint end,float restY,float restEnd){
    float t=(restY-p.y)/(restY-restEnd);
    return point(root.x+(end.x-root.x)*t+p.x*root.c,
                 root.y+(end.y-root.y)*t+p.x*root.s,p.z);
}
static Point gait_leg(Point p,int i){
    /* Blend around the knee so baggy fabric never splits at a bone boundary. */
    Point upper=gait_segment(p,gaitHip[i],gaitKnee[i],14.8f,7.4f);
    Point lower=gait_segment(p,gaitKnee[i],gaitAnkle[i],7.4f,.7f);
    float w=gait_smooth(geo_clamp((p.y-6.7f)/1.4f,0,1));
    float hipWeight=gait_smooth(geo_clamp((p.y-6.7f)/8.1f,0,1));
    Point result=point(lower.x+(upper.x-lower.x)*w,lower.y+(upper.y-lower.y)*w,p.z+poseSway*hipWeight);
    if(p.y<1.3f){
        /* Heel/toe roll stays above the support plane rather than sinking. */
        result.x=footStep[i]+p.x;
        result.y=p.y+footLift[i]+fabsf(p.x)*fabsf(footRoll[i]);
    }
    return result;
}
