#define _POSIX_C_SOURCE 200809L
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <ctype.h>
#include <stdarg.h>
#include <limits.h>
#include <errno.h>
#include <sys/stat.h>
#include <unistd.h>
#include <signal.h>
#ifdef __linux__
#include <X11/Xlib.h>
#include <X11/keysym.h>
#endif

#define FK_NAME_MAX 128
#define FK_MAX_LINE 16384
#define FK_INITIAL_CAP 16

typedef enum { V_NONE, V_NUM, V_STR, V_BOOL } ValueType;
typedef struct { ValueType type; long long num; int boolean; char *str; } Value;
typedef struct { char name[FK_NAME_MAX]; long long start,size,cap; Value *items; } Sell;
typedef struct { Sell *sells; size_t count,cap; } Env;

typedef struct {
    const char *src; size_t pos,len; Env *env;
} Expr;

#ifdef __linux__
typedef struct { Display *dpy; int screen; Window win; GC gc; int width,height,open; int mouse_x,mouse_y; unsigned int mouse_buttons; unsigned long fg,bg; int key_down[256]; } Gui;
static Gui G = {0};
#endif

static void fk_error(const char *fmt, ...){ va_list ap; fprintf(stderr,"FK error: "); va_start(ap,fmt); vfprintf(stderr,fmt,ap); va_end(ap); fputc('\n',stderr); exit(1); }
static void *xm(size_t n){ void*p=malloc(n?n:1); if(!p)fk_error("out of memory"); return p; }
static void *xr(void*p,size_t n){ void*q=realloc(p,n?n:1); if(!q)fk_error("out of memory"); return q; }
static char *xs(const char*s){ size_t n=strlen(s)+1; char*p=xm(n); memcpy(p,s,n); return p; }
static Value vn(long long n){Value v={V_NUM,n,0,NULL};return v;} static Value vb(int b){Value v={V_BOOL,0,b!=0,NULL};return v;} static Value vs(const char*s){Value v={V_STR,0,0,xs(s)};return v;} static Value vz(void){Value v={V_NONE,0,0,NULL};return v;}
static void vf(Value*v){if(v->type==V_STR)free(v->str);*v=vz();}
static Value vc(const Value*v){if(v->type==V_STR)return vs(v->str?v->str:"");return *v;}
static int truth(const Value*v){if(v->type==V_BOOL)return v->boolean; if(v->type==V_NUM)return v->num!=0; if(v->type==V_STR)return v->str&&*v->str; return 0;}
static long long num(const Value*v){if(v->type==V_NUM)return v->num;if(v->type==V_BOOL)return v->boolean; if(v->type==V_STR){char*e;long long n=strtoll(v->str,&e,10);if(e&&*e==0)return n;} fk_error("numeric value expected"); return 0;}
static void pv(const Value*v){switch(v->type){case V_NUM:printf("%lld",v->num);break;case V_BOOL:printf(v->boolean?"true":"false");break;case V_STR:printf("%s",v->str?v->str:"");break;default:printf("none");}}
static int veq(const Value*a,const Value*b){if(a->type==V_STR||b->type==V_STR)return a->type==V_STR&&b->type==V_STR&&strcmp(a->str?a->str:"",b->str?b->str:"")==0;return num(a)==num(b);}
static Sell*fs(Env*e,const char*n){for(size_t i=0;i<e->count;i++)if(!strcmp(e->sells[i].name,n))return &e->sells[i];return NULL;}
static Sell*gs(Env*e,const char*n){Sell*s=fs(e,n);if(s)return s;if(e->count==e->cap){e->cap=e->cap?e->cap*2:8;e->sells=xr(e->sells,e->cap*sizeof(Sell));}s=&e->sells[e->count++];memset(s,0,sizeof(*s));snprintf(s->name,sizeof(s->name),"%s",n);s->start=1;return s;}
static void si(Sell*s,long long a,long long b){if(b<a)fk_error("bad sell range");s->start=a;s->size=b-a+1;s->cap=0;s->items=NULL;long long c=FK_INITIAL_CAP;while(c<s->size)c*=2;s->items=xm((size_t)c*sizeof(Value));s->cap=c;for(long long i=0;i<c;i++)s->items[i]=vz();}
static void se(Sell*s,long long idx){if(idx<s->start)fk_error("index below sell start");if(idx>=s->start+s->size)s->size=idx-s->start+1;if(s->size>s->cap){long long c=s->cap?s->cap:FK_INITIAL_CAP;while(c<s->size)c*=2;long long old=s->cap;s->items=xr(s->items,(size_t)c*sizeof(Value));for(long long i=old;i<c;i++)s->items[i]=vz();s->cap=c;}}
static Value sg(Sell*s,long long idx){if(!s||idx<s->start||idx>=s->start+s->size)fk_error("sell index %lld not initialized",idx);return vc(&s->items[idx-s->start]);}
static void ss(Sell*s,long long idx,Value v){se(s,idx);vf(&s->items[idx-s->start]);s->items[idx-s->start]=v;}

#ifdef __linux__
static void gui_init(void){if(G.open)return; G.dpy=XOpenDisplay(NULL); if(!G.dpy){fprintf(stderr,"FK warning: X11 unavailable; GUI disabled\n");return;} G.screen=DefaultScreen(G.dpy);G.width=800;G.height=600;G.bg=WhitePixel(G.dpy,G.screen);G.fg=BlackPixel(G.dpy,G.screen);G.win=XCreateSimpleWindow(G.dpy,RootWindow(G.dpy,G.screen),50,50,G.width,G.height,1,G.fg,G.bg);XSelectInput(G.dpy,G.win,ExposureMask|KeyPressMask|KeyReleaseMask|ButtonPressMask|ButtonReleaseMask|PointerMotionMask|StructureNotifyMask);G.gc=XCreateGC(G.dpy,G.win,0,NULL);G.open=1;XMapWindow(G.dpy,G.win);XFlush(G.dpy);}
static void gui_pump(void){if(!G.open)return;while(XPending(G.dpy)){XEvent e;XNextEvent(G.dpy,&e);switch(e.type){case MotionNotify:G.mouse_x=e.xmotion.x;G.mouse_y=e.xmotion.y;break;case ButtonPress:if(e.xbutton.button==1)G.mouse_buttons|=1;if(e.xbutton.button==2)G.mouse_buttons|=2;if(e.xbutton.button==3)G.mouse_buttons|=4;G.mouse_x=e.xbutton.x;G.mouse_y=e.xbutton.y;break;case ButtonRelease:if(e.xbutton.button==1)G.mouse_buttons&=~1;if(e.xbutton.button==2)G.mouse_buttons&=~2;if(e.xbutton.button==3)G.mouse_buttons&=~4;break;case KeyPress:{KeySym k=XLookupKeysym(&e.xkey,0); if(k<256)G.key_down[k]=1;}break;case KeyRelease:{KeySym k=XLookupKeysym(&e.xkey,0); if(k<256)G.key_down[k]=0;}break;case ConfigureNotify:G.width=e.xconfigure.width;G.height=e.xconfigure.height;break;}}}
static int keycode_for(const char*s){if(!*s)return -1;if(strlen(s)==1)return (unsigned char)s[0];KeySym k=XStringToKeysym(s);return k<256?(int)k:-1;}
static int keyheld(const char*s){gui_pump();int k=keycode_for(s);return k>=0&&k<256&&G.key_down[k];}
#else
static void gui_init(void){} static void gui_pump(void){} static int keyheld(const char*s){(void)s;return 0;}
#endif

static void skipw(Expr*p){while(p->pos<p->len&&isspace((unsigned char)p->src[p->pos]))p->pos++;}
static int eat(Expr*p,const char*t){skipw(p);size_t n=strlen(t);if(p->pos+n<=p->len&&!strncmp(p->src+p->pos,t,n)){p->pos+=n;return 1;}return 0;}
static void ech(Expr*p,char c){skipw(p);if(p->pos>=p->len||p->src[p->pos]!=c)fk_error("expected '%c'",c);p->pos++;}
static Value expr(Expr*p);
static Value pstr(Expr*p){ech(p,'"');char*b=xm(64);size_t n=0,c=64;while(p->pos<p->len){char ch=p->src[p->pos++];if(ch=='"'){b[n]=0;Value v=vs(b);free(b);return v;}if(ch=='\\'&&p->pos<p->len){char e=p->src[p->pos++];if(e=='n')ch='\n';else if(e=='t')ch='\t';else ch=e;}if(n+2>c){c*=2;b=xr(b,c);}b[n++]=ch;}free(b);fk_error("unterminated string");return vz();}
static void pid(Expr*p,char*out){skipw(p);size_t n=0;while(p->pos<p->len&&(isalnum((unsigned char)p->src[p->pos])||p->src[p->pos]=='_' )){if(n+1<FK_NAME_MAX)out[n++]=p->src[p->pos];p->pos++;}if(!n)fk_error("identifier expected");out[n]=0;}

static Value primary(Expr*p){skipw(p);if(p->pos>=p->len)fk_error("unexpected end of expression");if(p->src[p->pos]=='"')return pstr(p);if(eat(p,"(")){Value v=expr(p);ech(p,')');return v;}if(isdigit((unsigned char)p->src[p->pos])){char*e;long long n=strtoll(p->src+p->pos,&e,10);p->pos=(size_t)(e-p->src);return vn(n);}char id[FK_NAME_MAX];pid(p,id);
#ifdef __linux__
    if(!strcmp(id,"mouse")){ if(eat(p,"=")){if(eat(p,"x")){gui_pump();return vn(G.mouse_x);}if(eat(p,"y")){gui_pump();return vn(G.mouse_y);}if(eat(p,"left")){gui_pump();return vb((G.mouse_buttons&1)!=0);}if(eat(p,"right")){gui_pump();return vb((G.mouse_buttons&4)!=0);}} }
    if(!strcmp(id,"keyboard")){ if(eat(p,"=")){if(eat(p,"hold")){ech(p,'(');Value k=pstr(p);ech(p,')');int r=keyheld(k.str?k.str:"");vf(&k);return vb(r);} if(p->pos<p->len&&p->src[p->pos]=='"'){Value k=pstr(p);int r=keyheld(k.str?k.str:"");vf(&k);return vb(r);} }}
#endif
    if(!strcmp(id,"true")||!strcmp(id,"white")||!strcmp(id,"yes"))return vb(1);
    if(!strcmp(id,"false")||!strcmp(id,"black")||!strcmp(id,"no"))return vb(0);
    skipw(p);size_t save=p->pos;if(eat(p,"=")){
        if(!eat(p,"sell"))fk_error("expected sell after %s =",id);
        if(!eat(p,"index"))fk_error("expected index");
        ech(p,'(');Value ix=expr(p);ech(p,')');long long i=num(&ix);vf(&ix);
        Sell*s=fs(p->env,id);if(!s)fk_error("unknown sell %s",id);return sg(s,i);
    }
    p->pos=save;
    Sell*s=fs(p->env,id);
    if(s && s->size>=1) return sg(s,1);
    fk_error("unknown value %s",id);return vz();}
static Value unary(Expr*p){if(eat(p,"!")){Value v=unary(p);int r=!truth(&v);vf(&v);return vb(r);}if(eat(p,"-")){Value v=unary(p);long long n=-num(&v);vf(&v);return vn(n);}return primary(p);}
static Value addv(Value a,Value b){if(a.type==V_STR||b.type==V_STR){char l[FK_MAX_LINE],r[FK_MAX_LINE]; if(a.type==V_STR)snprintf(l,sizeof(l),"%s",a.str?a.str:"");else if(a.type==V_NUM)snprintf(l,sizeof(l),"%lld",a.num);else snprintf(l,sizeof(l),"%s",truth(&a)?"true":"false");if(b.type==V_STR)snprintf(r,sizeof(r),"%s",b.str?b.str:"");else if(b.type==V_NUM)snprintf(r,sizeof(r),"%lld",b.num);else snprintf(r,sizeof(r),"%s",truth(&b)?"true":"false");size_t n=strlen(l)+strlen(r)+1;char*o=xm(n);snprintf(o,n,"%s%s",l,r);Value v=vs(o);free(o);vf(&a);vf(&b);return v;}long long n=num(&a)+num(&b);vf(&a);vf(&b);return vn(n);}
static Value mul(Expr*p){Value a=unary(p);for(;;){if(eat(p,"*")){Value b=unary(p);long long n=num(&a)*num(&b);vf(&a);vf(&b);a=vn(n);}else if(eat(p,"/")){Value b=unary(p);long long d=num(&b);if(!d)fk_error("division by zero");long long n=num(&a)/d;vf(&a);vf(&b);a=vn(n);}else if(eat(p,"%")){Value b=unary(p);long long d=num(&b);if(!d)fk_error("modulo by zero");long long n=num(&a)%d;vf(&a);vf(&b);a=vn(n);}else break;}return a;}
static Value plus(Expr*p){Value a=mul(p);for(;;){if(eat(p,"+")){Value b=mul(p);a=addv(a,b);}else if(eat(p,"-")){Value b=mul(p);long long n=num(&a)-num(&b);vf(&a);vf(&b);a=vn(n);}else break;}return a;}
static Value cmp(Expr*p){Value a=plus(p);for(;;){int op=0;if(eat(p,"=="))op=1;else if(eat(p,"!="))op=2;else if(eat(p,">="))op=3;else if(eat(p,"<="))op=4;else if(eat(p,">"))op=5;else if(eat(p,"<"))op=6;else break;Value b=plus(p);int r=0;if(op==1)r=veq(&a,&b);else if(op==2)r=!veq(&a,&b);else{long long x=num(&a),y=num(&b);r=op==3?x>=y:op==4?x<=y:op==5?x>y:x<y;}vf(&a);vf(&b);a=vb(r);}return a;}
static Value land(Expr*p){Value a=cmp(p);while(eat(p,"&&")){Value b=cmp(p);int r=truth(&a)&&truth(&b);vf(&a);vf(&b);a=vb(r);}return a;} static Value expr(Expr*p){Value a=land(p);while(eat(p,"||")){Value b=land(p);int r=truth(&a)||truth(&b);vf(&a);vf(&b);a=vb(r);}return a;}
static void done(Expr*p){skipw(p);if(p->pos!=p->len)fk_error("unexpected expression near: %s",p->src+p->pos);}

static int commentline(const char*s){while(*s&&isspace((unsigned char)*s))s++;return !strncmp(s,"//;;##",6);} 
static const char* skipstr(const char*p,const char*e){p++;while(p<e){if(*p=='\\'&&p+1<e){p+=2;continue;}if(*p=='"')return p+1;p++;}fk_error("unterminated string");return e;}
static const char* match(const char*o,const char*e,char l,char r){int d=0;for(const char*p=o;p<e;p++){if(*p=='"'){p=skipstr(p,e)-1;continue;}if(*p==l)d++;else if(*p==r&&--d==0)return p;}fk_error("unclosed block");return e;}
static void trim(const char**a,const char**b){while(*a<*b&&isspace((unsigned char)**a))(*a)++;while(*b>*a&&isspace((unsigned char)*((*b)-1)))(*b)--;}
static void execsrc(Env*e,const char*src);


static void fileop(Env*e,const char*stmt){const char*p=stmt+4;while(isspace((unsigned char)*p))p++;char name[FK_NAME_MAX];if(*p!='(')fk_error("expected Sell (name)");p++;const char*q=strchr(p,')');if(!q)fk_error("missing sell name )");size_t nn=(size_t)(q-p);if(nn>=sizeof(name))fk_error("sell name too long");memcpy(name,p,nn);name[nn]=0;p=q+1;while(isspace((unsigned char)*p))p++;if(strncmp(p,"index",5))fk_error("expected index");p+=5;while(isspace((unsigned char)*p))p++;if(*p!='(')fk_error("expected index (");q=strchr(p,')');if(!q)fk_error("missing index )");char *ix=xm((size_t)(q-p));memcpy(ix,p+1,(size_t)(q-p-1));ix[q-p-1]=0;Expr ep={ix,0,strlen(ix),e};Value iv=expr(&ep);done(&ep);long long idx=num(&iv);vf(&iv);free(ix);p=q+1;while(isspace((unsigned char)*p))p++;if(strncmp(p,"computer",8))fk_error("expected computer");p+=8;while(isspace((unsigned char)*p))p++;if(*p!='(')fk_error("expected computer (");q=strrchr(p,')');if(!q)fk_error("missing computer )");char path[PATH_MAX];size_t pn=(size_t)(q-p-1);if(pn>=sizeof(path))fk_error("path too long");memcpy(path,p+1,pn);path[pn]=0;p=q+1;while(isspace((unsigned char)*p))p++;Sell*s=fs(e,name);if(!s)fk_error("unknown sell %s",name);Value v=sg(s,idx);if(!strcmp(p,"Make")){FILE*f=fopen(path,"wb");if(!f){vf(&v);fk_error("cannot create %s: %s",path,strerror(errno));}if(v.type==V_STR)fprintf(f,"%s",v.str?v.str:"");else if(v.type==V_NUM)fprintf(f,"%lld",v.num);else if(v.type==V_BOOL)fprintf(f,"%s",v.boolean?"true":"false");fclose(f);}else if(!strcmp(p,"to cover up")){FILE*f=fopen(path,"wb");if(!f){vf(&v);fk_error("cannot overwrite %s: %s",path,strerror(errno));}if(v.type==V_STR)fprintf(f,"%s",v.str?v.str:"");else if(v.type==V_NUM)fprintf(f,"%lld",v.num);else if(v.type==V_BOOL)fprintf(f,"%s",v.boolean?"true":"false");fclose(f);}else if(!strcmp(p,"Kill")){if(unlink(path)!=0){vf(&v);fk_error("cannot delete %s: %s",path,strerror(errno));}}else fk_error("unknown file operation: %s",p);vf(&v);}

static void http_get(const char*url){char esc[FK_MAX_LINE*2];size_t n=0;for(const char*p=url;*p&&n+5<sizeof(esc);p++){if(*p=='\''){esc[n++]='\'';esc[n++]='"';esc[n++]='\'';esc[n++]='"';esc[n++]='\'';}else esc[n++]=*p;}esc[n]=0;char cmd[FK_MAX_LINE*2+128];snprintf(cmd,sizeof(cmd),"curl -L --fail --silent --show-error '%s'",esc);int rc=system(cmd);if(rc!=0)fk_error("API request failed");}

#ifdef __linux__
static void gui_cmd(const char*stmt){if(!strncmp(stmt,"gui window",10)){const char*p=strchr(stmt,'(');if(!p){gui_init();return;}int w=0,h=0;if(sscanf(p,"(%d,%d)",&w,&h)!=2)fk_error("gui window requires (width,height)");G.width=w;G.height=h;gui_init();XResizeWindow(G.dpy,G.win,w,h);{const char*tp=strstr(stmt,"title");if(tp){const char*lp=strchr(tp,'(');const char*rp=lp?strchr(lp,')'):NULL;if(lp&&rp&&rp>lp){char*t=xm((size_t)(rp-lp));memcpy(t,lp+1,(size_t)(rp-lp-1));t[rp-lp-1]=0;if(t[0]=='"'){memmove(t,t+1,strlen(t));char*z=strrchr(t,'"');if(z)*z=0;}XStoreName(G.dpy,G.win,t);free(t);}}}XFlush(G.dpy);return;}if(!strcmp(stmt,"gui window Show")){gui_init();XMapWindow(G.dpy,G.win);XFlush(G.dpy);return;}if(!strcmp(stmt,"gui window Kill")){if(G.open){XDestroyWindow(G.dpy,G.win);XCloseDisplay(G.dpy);memset(&G,0,sizeof(G));}return;}if(!strncmp(stmt,"gui rect",8)){gui_init();int x,y,w,h;if(sscanf(strchr(stmt,'('),"(%d,%d,%d,%d)",&x,&y,&w,&h)!=4)fk_error("gui rect requires (x,y,w,h)");XFillRectangle(G.dpy,G.win,G.gc,x,y,w,h);XFlush(G.dpy);return;}if(!strncmp(stmt,"gui text",8)){gui_init();const char*p=strchr(stmt,'('),*q=strrchr(stmt,')');if(!p||!q||q<=p)fk_error("gui text requires (text)");char*t=xm((size_t)(q-p));memcpy(t,p+1,(size_t)(q-p-1));t[q-p-1]=0;if(*t=='"'){memmove(t,t+1,strlen(t));char*z=strrchr(t,'"');if(z)*z=0;}int x=10,y=20;const char*po=strstr(q,"position");if(po)sscanf(po,"position (%d,%d)",&x,&y);XDrawString(G.dpy,G.win,G.gc,x,y,t,(int)strlen(t));XFlush(G.dpy);free(t);return;}fk_error("unknown gui command: %s",stmt);}
#endif
static void audio_cmd(const char*stmt){const char*p=strchr(stmt,'(');if(!p)fk_error("audio requires (file)");const char*q=strrchr(stmt,')');if(!q)fk_error("audio missing )");char path[PATH_MAX];size_t n=(size_t)(q-p-1);if(n>=sizeof(path))fk_error("audio path too long");memcpy(path,p+1,n);path[n]=0;if(path[0]=='"'){memmove(path,path+1,strlen(path));char*z=strrchr(path,'"');if(z)*z=0;}if(system("command -v aplay >/dev/null 2>&1")!=0 && system("command -v paplay >/dev/null 2>&1")!=0 && system("command -v ffplay >/dev/null 2>&1")!=0)fk_error("no audio backend (aplay/paplay/ffplay)");char cmd[PATH_MAX+64];if(!strncmp(q+1," effect",7)||!strcmp(q+1," play")||!strcmp(q+1," effect")){snprintf(cmd,sizeof(cmd),"if command -v aplay >/dev/null 2>&1; then aplay -q \"%s\"; elif command -v paplay >/dev/null 2>&1; then paplay \"%s\"; else ffplay -nodisp -autoexit -loglevel quiet \"%s\"; fi",path,path,path);system(cmd);}else if(strstr(q,"stop")){system("true");}else fk_error("unsupported audio command");}

static void statement(Env*e,const char*a,const char*b){trim(&a,&b);if(a>=b)return;size_t n=(size_t)(b-a);char*s=xm(n+1);memcpy(s,a,n);s[n]=0;if(commentline(s)){free(s);return;}if(strstr(s,"//")&&strncmp(s,"//;;##",6)!=0)fk_error("inline comments are not allowed");
    if(!strncmp(s,"Sell ",5)){fileop(e,s);free(s);return;} if(!strncmp(s,"https://",8)){http_get(s);free(s);return;}
#ifdef __linux__
    if(!strncmp(s,"gui ",4)){gui_cmd(s);free(s);return;}
#endif
    if(!strncmp(s,"audio",5)){audio_cmd(s);free(s);return;}
    if(!strncmp(s,"prinf",5)){const char*p=strchr(s,'(');const char*q=strrchr(s,')');if(!p||!q||q<p)fk_error("bad prinf");char*x=xm((size_t)(q-p));memcpy(x,p+1,(size_t)(q-p-1));x[q-p-1]=0;Expr ep={x,0,strlen(x),e};Value v=expr(&ep);done(&ep);pv(&v);putchar('\n');vf(&v);free(x);free(s);return;}
    if(!strncmp(s,"if ",3)||!strncmp(s,"if{",3)){const char*lb=strchr(s,'{');if(!lb)fk_error("if requires block");const char*rb=match(lb,s+n,'{','}');const char*ca=s+2,*cb=lb;trim(&ca,&cb);Expr ep={ca,0,(size_t)(cb-ca),e};Value v=expr(&ep);done(&ep);int yes=truth(&v);vf(&v);if(yes){size_t bl=(size_t)(rb-lb-1);char*body=xm(bl+1);memcpy(body,lb+1,bl);body[bl]=0;execsrc(e,body);free(body);}const char*rest=rb+1;while(*rest&&isspace((unsigned char)*rest))rest++;if(!strncmp(rest,"else",4)){rest+=4;while(*rest&&isspace((unsigned char)*rest))rest++;if(*rest!='{')fk_error("else requires block");const char*er=match(rest,s+n,'{','}');if(!yes){size_t bl=(size_t)(er-rest-1);char*body=xm(bl+1);memcpy(body,rest+1,bl);body[bl]=0;execsrc(e,body);free(body);}}free(s);return;}
    if(s[0]=='['){
        const char*cl=match(s,s+n,'[',']');
        const char*p=cl+1; while(*p&&isspace((unsigned char)*p))p++;
        if(*p!='(')fk_error("repeat requires count");
        const char*rp=match(p,s+n,'(',')');
        char*x=xm((size_t)(rp-p)); memcpy(x,p+1,(size_t)(rp-p-1)); x[rp-p-1]=0;
        Expr ep={x,0,strlen(x),e}; Value v=expr(&ep); done(&ep);
        long long c=num(&v); vf(&v); if(c<0)fk_error("negative repeat");
        size_t bl=(size_t)(cl-s-1); char*body=xm(bl+1); memcpy(body,s+1,bl); body[bl]=0;
        for(long long i=0;i<c;i++){
            execsrc(e,body);
#ifdef __linux__
            gui_pump();
#endif
        }
        free(body); free(x); free(s); return;
    }
    if(strncmp(s,"else",4)==0)fk_error("else must follow if");
    char*eq=strchr(s,'=');if(!eq)fk_error("unknown statement: %s",s);const char*na=s,*nb=eq;trim(&na,&nb);if((size_t)(nb-na)>=FK_NAME_MAX)fk_error("name too long");char name[FK_NAME_MAX];memcpy(name,na,(size_t)(nb-na));name[nb-na]=0;for(char*c=name;*c;c++)if(!(isalnum((unsigned char)*c)||*c=='_'))fk_error("invalid name %s",name);const char*rhs=eq+1;while(isspace((unsigned char)*rhs))rhs++;
    if(!strncmp(rhs,"sell",4)&&isspace((unsigned char)rhs[4])){rhs+=4;while(isspace((unsigned char)*rhs))rhs++;if(!strncmp(rhs,"make",4)){rhs+=4;while(isspace((unsigned char)*rhs))rhs++;Sell*sell=gs(e,name);if(!*rhs){free(sell->items);sell->items=NULL;sell->cap=sell->size=0;sell->start=1;}else{char*en;long long a=strtoll(rhs,&en,10);while(isspace((unsigned char)*en))en++;if(*en!='~')fk_error("sell range needs ~");long long b=strtoll(en+1,&en,10);while(isspace((unsigned char)*en))en++;if(*en)fk_error("bad sell range");free(sell->items);sell->items=NULL;si(sell,a,b);}free(s);return;}}
    if(!strncmp(rhs,"index",5)){rhs+=5;while(isspace((unsigned char)*rhs))rhs++;if(*rhs!='(')fk_error("index (");const char*rp=strchr(rhs,')');if(!rp)fk_error("index )");char*x=xm((size_t)(rp-rhs));memcpy(x,rhs+1,(size_t)(rp-rhs-1));x[rp-rhs-1]=0;Expr ip={x,0,strlen(x),e};Value iv=expr(&ip);done(&ip);long long idx=num(&iv);vf(&iv);free(x);const char*gt=rp+1;while(isspace((unsigned char)*gt))gt++;if(*gt!='>')fk_error("index assignment needs >");gt++;while(isspace((unsigned char)*gt))gt++;Expr ep={gt,0,strlen(gt),e};Value v=expr(&ep);done(&ep);ss(gs(e,name),idx,v);free(s);return;}
    Expr ep={rhs,0,strlen(rhs),e};Value v=expr(&ep);done(&ep);
    /* scalar variables are represented internally as Sell index 1 */
    ss(gs(e,name),1,v);free(s);
}

static void execsrc(Env*e,const char*src){const char*p=src,*ls=src;int bd=0,br=0,pa=0;while(*p){if(*p=='"'){p=skipstr(p,p+strlen(p))-1;}else{if(*p=='{')bd++;else if(*p=='}')bd--;else if(*p=='[')br++;else if(*p==']')br--;else if(*p=='(')pa++;else if(*p==')')pa--;if(*p=='\n'&&bd==0&&br==0&&pa==0){statement(e,ls,p);ls=p+1;}}p++;}statement(e,ls,p);} 


static void destroy_env(Env*e){for(size_t i=0;i<e->count;i++){Sell*s=&e->sells[i];for(long long j=0;j<s->cap;j++)vf(&s->items[j]);free(s->items);}free(e->sells);}
static char* readfile(const char*path){FILE*f=fopen(path,"rb");if(!f)return NULL;fseek(f,0,SEEK_END);long n=ftell(f);fseek(f,0,SEEK_SET);char*b=xm((size_t)n+1);if(fread(b,1,(size_t)n,f)!=(size_t)n){fclose(f);free(b);return NULL;}b[n]=0;fclose(f);return b;}
int main(int argc,char**argv){if(argc<2){fprintf(stderr,"FK 2.0\nusage: %s file.fk\n",argv[0]);return 1;}char*src=readfile(argv[1]);if(!src){perror(argv[1]);return 1;}Env e={0};execsrc(&e,src);free(src);destroy_env(&e);return 0;}
