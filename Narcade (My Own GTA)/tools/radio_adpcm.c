/* v2.52 (Claude): codificador IMA ADPCM por bloques para la radio de los carros.
   Entrada: PCM s16 mono por stdin. Salida: bloques de 4+1024 bytes (2048 muestras) por stdout.
   Cabecera de bloque: predictor int16, indice uint8, 0. Cada bloque se decodifica por separado
   (asi el juego puede empezar una cancion en cualquier segundo). Lo usa tools/build_radio.py. */
#include <stdio.h>
#include <stdint.h>
#include <string.h>
#ifdef _WIN32
#include <io.h>
#include <fcntl.h>
#endif
#define BLOCK 2048
static const int steps[89]={7,8,9,10,11,12,13,14,16,17,19,21,23,25,28,31,34,37,41,45,50,55,60,66,73,80,88,97,107,118,
 130,143,157,173,190,209,230,253,279,307,337,371,408,449,494,544,598,658,724,796,876,963,1060,1166,1282,1411,1552,
 1707,1878,2066,2272,2499,2749,3024,3327,3660,4026,4428,4871,5358,5894,6484,7132,7845,8630,9493,10442,11487,12635,
 13899,15289,16818,18500,20350,22385,24623,27086,29794,32767};
static const int adjust[8]={-1,-1,-1,-1,2,4,6,8};
int main(void){
#ifdef _WIN32
    _setmode(_fileno(stdin),_O_BINARY);_setmode(_fileno(stdout),_O_BINARY);
#endif
    int16_t in[BLOCK];int pred=0,index=0;
    for(;;){
        size_t n=fread(in,2,BLOCK,stdin);if(n==0)break;
        if(n<BLOCK)memset(in+n,0,(BLOCK-n)*2);
        uint8_t out[4+BLOCK/2];out[0]=(uint8_t)(pred&255);out[1]=(uint8_t)((pred>>8)&255);out[2]=(uint8_t)index;out[3]=0;
        for(int i=0;i<BLOCK;i++){
            int step=steps[index],diff=in[i]-pred,code=0;if(diff<0){code=8;diff=-diff;}
            int delta=step>>3;
            if(diff>=step){code|=4;diff-=step;delta+=step;}step>>=1;
            if(diff>=step){code|=2;diff-=step;delta+=step;}step>>=1;
            if(diff>=step){code|=1;delta+=step;}
            pred+=(code&8)?-delta:delta;if(pred>32767)pred=32767;if(pred<-32768)pred=-32768;
            index+=adjust[code&7];if(index<0)index=0;if(index>88)index=88;
            if(i&1)out[4+i/2]|=(uint8_t)(code<<4);else out[4+i/2]=(uint8_t)code;
        }
        fwrite(out,1,sizeof out,stdout);
        if(n<BLOCK)break;
    }
    return 0;
}
