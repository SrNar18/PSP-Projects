/* Broad, continuous color variation in world coordinates. Shared edges have
   exactly the same color; the city cache records it once, with no new draws. */
static float street_hash(int x,int z){
    unsigned h=(unsigned)x*374761393u+(unsigned)z*668265263u;
    h=(h^(h>>13))*1274126177u;h^=h>>16;
    return (h&65535)/65535.f;
}
static uint32_t street_tint(uint32_t color,float x,float z){
    float fx=x/192.f,fz=z/192.f;int ix=(int)floorf(fx),iz=(int)floorf(fz);
    float u=fx-ix,v=fz-iz;u=u*u*(3-2*u);v=v*v*(3-2*v);
    float a=street_hash(ix,iz)*(1-u)+street_hash(ix+1,iz)*u;
    float b=street_hash(ix,iz+1)*(1-u)+street_hash(ix+1,iz+1)*u;
    float shade=.94f+.12f*(a*(1-v)+b*v);
    int r=(int)((color&255)*shade),g=(int)(((color>>8)&255)*shade),bl=(int)(((color>>16)&255)*shade);
    return COLOR(r>255?255:r,g>255?255:g,bl>255?255:bl);
}
