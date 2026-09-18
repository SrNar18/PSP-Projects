#ifndef PSPIA_SEARCH_H
#define PSPIA_SEARCH_H
typedef struct {unsigned doc;float score;} KbHit;
int kb_open(const char *dir);
unsigned kb_docs(void);
unsigned kb_terms(void);
int kb_words(const char *text,char out[][24],int max);
int kb_search(const char *query,KbHit *hits,int max,char qwords[][24],int *nq);
int kb_doc(unsigned doc,char *title,int tcap,char *text,int cap);
int kb_variant(const char *w,char *out);
int kb_best_answer_t(const char *title,const char *text,char qwords[][24],int nq,const char *question,char *out,int cap);
int kb_best_answer(const char *text,char qwords[][24],int nq,const char *question,char *out,int cap);
int kb_best_sentence(const char *text,char qwords[][24],int nq,char *out,int cap);
#endif
