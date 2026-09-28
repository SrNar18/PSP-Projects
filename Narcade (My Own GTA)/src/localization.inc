/* Exact, generated UTF-8 source catalog; displayed bitmap text is ASCII. */
#include <stdarg.h>
#include "localization_catalog.h"
static int prefsLanguage;
static const char *locale_text(const char *s){
    if(!s||!prefsLanguage)return s?s:"";
    int lo=0,hi=(int)(sizeof(localeCatalog)/sizeof(localeCatalog[0]))-1;
    while(lo<=hi){int mid=(lo+hi)/2,c=strcmp(s,localeCatalog[mid].es);
        if(!c)return localeCatalog[mid].en;if(c<0)hi=mid-1;else lo=mid+1;}
    return s;
}
const char *game_localize(const char *s){return locale_text(s);}
int game_language(void){return prefsLanguage;}
/* File paths and identifiers must never enter the translation catalog. */
static int raw_snprintf(char *out,size_t cap,const char *format,...){
    va_list args;va_start(args,format);int n=vsnprintf(out,cap,format,args);va_end(args);return n;
}
/* Reject unsupported printf features before consuming ANY arguments. */
static int locale_format_valid(const char *f){
    while(*f){if(*f++!='%')continue;if(*f=='%'){f++;continue;}
        while(*f&&strchr("-+ #0.123456789",*f))f++;
        if(!*f||!strchr("sdiucxXofFgGeE",*f))return 0;f++;
    }return 1;
}
/* Translate the format BEFORE substitution and every string argument, so
   dynamic mission names, clues, directions and counters use the same language.
   The game uses only these standard, non-positional printf conversions. */
static int locale_snprintf(char *out,size_t cap,const char *format,...){
    const char *f=locale_text(format);if(!locale_format_valid(f)){if(cap)out[0]=0;return -1;}
    va_list args;va_start(args,format);size_t total=0;
    while(*f){char part[1024];int n=0;
        if(*f!='%'){part[0]=*f++;part[1]=0;n=1;}
        else if(f[1]=='%'){part[0]='%';part[1]=0;f+=2;n=1;}
        else{char spec[32];int k=0;spec[k++]=*f++;
            while(*f&&strchr("-+ #0.123456789",*f)&&k<28)spec[k++]=*f++;
            char type=*f;if(type)f++;spec[k++]=type;spec[k]=0;
            if(type=='s')n=snprintf(part,sizeof part,spec,locale_text(va_arg(args,const char*)));
            else if(type=='d'||type=='i'||type=='c')n=snprintf(part,sizeof part,spec,va_arg(args,int));
            else if(type=='u'||type=='x'||type=='X'||type=='o')n=snprintf(part,sizeof part,spec,va_arg(args,unsigned));
            else if(strchr("fFgGeE",type))n=snprintf(part,sizeof part,spec,va_arg(args,double));
            else{part[0]='?';part[1]=0;n=1;}
        }
        if(n<0)n=0;size_t available=cap&&total<cap-1?cap-1-total:0;
        size_t copy=(size_t)n;if(copy>sizeof part-1)copy=sizeof part-1;if(copy>available)copy=available;
        if(copy)memcpy(out+total,part,copy);total+=(size_t)n;
    }
    if(cap)out[total<cap?total:cap-1]=0;va_end(args);return (int)total;
}
#define snprintf locale_snprintf
