#ifndef PSPIA_CHAT_H
#define PSPIA_CHAT_H
int lm_load(const char *path);
int lm_ready(void);
int lm_generate(const char *prompt,char *out,int cap,int maxBytes,float temp,void (*tick)(const char *partial));
#endif
