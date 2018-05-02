/**********************************************************************
 * 
 * This file is part of the AWeb-II distribution
 *
 * Copyright (C) 2002 Yvon Rozijn
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the AWeb Public License as included in this
 * distribution.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * AWeb Public License for more details.
 *
 **********************************************************************/

/* jslib.c - AWeb js awebplugin library module */

#include "awebjs.h"
#include "jprotos.h"
#include "keyfile.h"
#include <libraries/locale.h>
#include <intuition/intuition.h>
#include <intuition/intuitionbase.h>
#include <exec/resident.h>
#include <graphics/gfxbase.h>
#include <classact.h>
#include <clib/exec_protos.h>
#include <clib/intuition_protos.h>
#include <pragmas/intuition_pragmas.h>
#include <time.h>

#define PUDDLESIZE         16*1024
#define TRESHSIZE          4*1024

struct ExecBase *SysBase;
struct Library *DOSBase,*LocaleBase,*UtilityBase,*IntuitionBase,*GfxBase,*AslBase;
struct Locale *locale;
struct Hook idcmphook;

/* Runtime error values */
#define JERRORS_CONTINUE   -1 /* Don't show errors and try to continue script */
#define JERRORS_OFF        0  /* Don't show errors and stop script */
#define JERRORS_ON         1  /* Show errors and stop script */

/*-----------------------------------------------------------------------*/
/* AWebLib module startup */

__asm __saveds struct Library *Initlib(
   register __a6 struct ExecBase *sysbase,
   register __a0 struct SegList *seglist,
   register __d0 struct Library *libbase);

__asm __saveds struct Library *Openlib(
   register __a6 struct Library *libbase);

__asm __saveds struct SegList *Closelib(
   register __a6 struct Library *libbase);

__asm __saveds struct SegList *Expungelib(
   register __a6 struct Library *libbase);

__asm __saveds ULONG Extfunclib(void);

__asm __saveds void *Newjcontext(
   register __a0 UBYTE *screenname);

__asm __saveds void Freejcontext(
   register __a0 struct Jcontext *jc);

__asm __saveds BOOL Runjprogram(
   register __a0 struct Jcontext *jc,
   register __a1 struct Jobject *fscope,
   register __a2 UBYTE *source,
   register __a3 struct Jobject *jthis,
   register __a4 struct Jobject **gwtab,
   register __d0 ULONG protkey,
   register __d1 ULONG userdata);

__asm __saveds void *Newjobject(
   register __a0 struct Jcontext *jc);

__asm __saveds void Disposejobject(
   register __a0 struct Jobject *jo);

__asm __saveds struct Jobject *AddjfunctionA(
   register __a0 struct Jcontext *jc,
   register __a1 struct Jobject *jo,
   register __a2 UBYTE *name,
   register __a3 void (*code)(void *),
   register __a4 UBYTE **args);

__asm __saveds struct Variable *Jfargument(
   register __a0 struct Jcontext *jc,
   register __d0 long n);

__asm __saveds UBYTE *Jtostring(
   register __a0 struct Jcontext *jc,
   register __a1 struct Variable *jv);

__asm __saveds void Jasgstring(
   register __a0 struct Jcontext *jc,
   register __a1 struct Variable *jv,
   register __a2 UBYTE *string);

__asm __saveds void Jasgobject(
   register __a0 struct Jcontext *jc,
   register __a1 struct Variable *jv,
   register __a2 struct Jobject *jo);

__asm __saveds void Setjobject(
   register __a0 struct Jobject *jo,
   register __a1 Objhookfunc *hook,
   register __a2 void *internal,
   register __a3 void (*dispose)(void *));

__asm __saveds struct Variable *Jproperty(
   register __a0 struct Jcontext *jc,
   register __a1 struct Jobject *jo,
   register __a2 UBYTE *name);

__asm __saveds void Setjproperty(
   register __a0 struct Variable *jv,
   register __a1 Varhookfunc *hook,
   register __a2 void *hookdata);

__asm __saveds struct Jobject *Jthis(
   register __a0 struct Jcontext *jc);

__asm __saveds void *Jointernal(
   register __a0 struct Jobject *jo);

__asm __saveds void Jasgboolean(
   register __a0 struct Jcontext *jc,
   register __a1 struct Variable *jv,
   register __d0 BOOL bvalue);

__asm __saveds BOOL Jtoboolean(
   register __a0 struct Jcontext *jc,
   register __a1 struct Variable *jv);

__asm __saveds struct Jobject *Newjarray(
   register __a0 struct Jcontext *jc);

__asm __saveds struct Variable *Jnewarrayelt(
   register __a0 struct Jcontext *jc,
   register __a1 struct Jobject *jo);

__asm __saveds struct Jobject *Jtoobject(
   register __a0 struct Jcontext *jc,
   register __a1 struct Variable *jv);

__asm __saveds long Jtonumber(
   register __a0 struct Jcontext *jc,
   register __a1 struct Variable *jv);

__asm __saveds void Jasgnumber(
   register __a0 struct Jcontext *jc,
   register __a1 struct Variable *jv,
   register __d0 long nvalue);

__asm __saveds BOOL Jisarray(
   register __a0 struct Jcontext *jc,
   register __a1 struct Jobject *jo);

__asm __saveds struct Jobject *Jfindarray(
   register __a0 struct Jcontext *jc,
   register __a1 struct Jobject *jo,
   register __a2 UBYTE *name);

__asm __saveds void Jsetprototype(
   register __a0 struct Jcontext *jc,
   register __a1 struct Jobject *jo,
   register __a2 struct Jobject *proto);

__asm __saveds ULONG Jgetuserdata(
   register __a0 struct Jcontext *jc);

__asm __saveds BOOL Jisnumber(
   register __a0 struct Variable *jv);

__asm __saveds void Clearjobject(
   register __a0 struct Jobject *jo,
   register __a1 UBYTE **except);

__asm __saveds void Freejobject(
   register __a0 struct Jobject *jo);

__asm __saveds void Jdumpobjects(
   register __a0 struct Jcontext *jc);

__asm __saveds struct Variable *Jgetreturnvalue(
   register __a0 struct Jcontext *jc);

__asm __saveds void Jpprotect(
   register __a0 struct Variable *var,
   register __d0 ULONG protkey);

__asm __saveds void Jcprotect(
   register __a0 struct Jcontext *jc,
   register __d0 ULONG protkey);

__asm __saveds UBYTE *Jpname(
   register __a0 struct Variable *var);

__asm __saveds void *Jdisposehook(
   register __a0 struct Jobject *jo);

__asm __saveds void Jsetfeedback(
   register __a0 struct Jcontext *jc,
   register __a1 Jfeedback *jf);

__asm __saveds void Jdebug(
   register __a0 struct Jcontext *jc,
   register __d0 BOOL debugon);

__asm __saveds void Jerrors(
   register __a0 struct Jcontext *jc,
   register __d0 BOOL comperrors,
   register __d1 long runerrors,
   register __d2 BOOL watch);

__asm __saveds void Jkeepobject(
   register __a0 struct Jobject *jo,
   register __d0 BOOL used);

__asm __saveds void Jgarbagecollect(
   register __a0 struct Jcontext *jc);

__asm __saveds void Jsetlinenumber(
   register __a0 struct Jcontext *jc,
   register __d0 long linenr);

__asm __saveds void Jsetobjasfunc(
   register __a0 struct Jobject *jo,
   register __d0 BOOL asfunc);

__asm __saveds void Jsetscreen(
   register __a0 struct Jcontext *jc,
   register __a1 UBYTE *screenname);

__asm __saveds void Jaddeventhandler(
   register __a0 struct Jcontext *jc,
   register __a1 struct Jobject *jo,
   register __a2 UBYTE *name,
   register __a3 UBYTE *source);

/* Function declarations for project dependent hook functions */
static ULONG Initaweblib(struct Library *libbase);
static void Expungeaweblib(struct Library *libbase);

struct Library *AWebJSBase;

static APTR libseglist;

LONG __saveds __asm Libstart(void)
{  return -1;
}

static APTR functable[]=
{  Openlib,
   Closelib,
   Expungelib,
   Extfunclib,
   Newjcontext,
   Freejcontext,
   Runjprogram,
   Newjobject,
   Disposejobject,
   AddjfunctionA,
   Jfargument,
   Jtostring,
   Jasgstring,
   Jasgobject,
   Setjobject,
   Jproperty,
   Setjproperty,
   Jthis,
   Jointernal,
   Jasgboolean,
   Jtoboolean,
   Newjarray,
   Jnewarrayelt,
   Jtoobject,
   Jtonumber,
   Jasgnumber,
   Jisarray,
   Jfindarray,
   Jsetprototype,
   Jgetuserdata,
   Jisnumber,
   Clearjobject,
   Freejobject,
   Jdumpobjects,
   Jgetreturnvalue,
   Jpprotect,
   Jcprotect,
   Jpname,
   Jdisposehook,
   Jsetfeedback,
   Jdebug,
   Jerrors,
   Jkeepobject,
   Jgarbagecollect,
   Jsetlinenumber,
   Jsetobjasfunc,
   Jsetscreen,
   Jaddeventhandler,
   (APTR)-1
};

/* Init table used in library initialization. */
static ULONG inittab[]=
{  sizeof(struct Library),
   (ULONG) functable,
   0,
   (ULONG) Initlib
};

static char __aligned libname[]="awebjs.aweblib";
static char __aligned libid[]="awebjs.aweblib " AWEBLIBVSTRING " " __AMIGADATE__;

/* The ROM tag */
struct Resident __aligned romtag=
{  RTC_MATCHWORD,
   &romtag,
   &romtag+1,
   RTF_AUTOINIT,
   AWEBLIBVERSION,
   NT_LIBRARY,
   0,
   libname,
   libid,
   inittab
};

__asm __saveds struct Library *Initlib(
   register __a6 struct ExecBase *sysbase,
   register __a0 struct SegList *seglist,
   register __d0 struct Library *libbase)
{  SysBase=sysbase;
   AWebJSBase=libbase;
   libbase->lib_Revision=AWEBLIBREVISION;
   libseglist=seglist;
   if(!Initaweblib(libbase))
   {  Expungeaweblib(libbase);
      libbase=NULL;
   }
   return libbase;
}

__asm __saveds struct Library *Openlib(
   register __a6 struct Library *libbase)
{  libbase->lib_OpenCnt++;
   libbase->lib_Flags&=~LIBF_DELEXP;
   return libbase;
}

__asm __saveds struct SegList *Closelib(
   register __a6 struct Library *libbase)
{  libbase->lib_OpenCnt--;
   if(libbase->lib_OpenCnt==0)
   {  if(libbase->lib_Flags&LIBF_DELEXP)
      {  return Expungelib(libbase);
      }
   }
   return NULL;
}

__asm __saveds struct SegList *Expungelib(
   register __a6 struct Library *libbase)
{  if(libbase->lib_OpenCnt==0)
   {  ULONG size=libbase->lib_NegSize+libbase->lib_PosSize;
      UBYTE *ptr=(UBYTE *)libbase-libbase->lib_NegSize;
      Remove((struct Node *)libbase);
      Expungeaweblib(libbase);
      FreeMem(ptr,size);
      return libseglist;
   }
   libbase->lib_Flags|=LIBF_DELEXP;
   return NULL;
}

__asm __saveds ULONG Extfunclib(void)
{  return 0;
}

/*-----------------------------------------------------------------------*/

static ULONG Initaweblib(struct Library *libbase)
{  if(!(DOSBase=OpenLibrary("dos.library",39))) return FALSE;
   if(!(LocaleBase=OpenLibrary("locale.library",0))) return FALSE;
   if(!(UtilityBase=OpenLibrary("utility.library",39))) return FALSE;
   if(!(IntuitionBase=OpenLibrary("intuition.library",39))) return FALSE;
   if(!(GfxBase=OpenLibrary("graphics.library",39))) return FALSE;
   if(!(AslBase=OpenLibrary("asl.library",OSNEED(39,44)))) return FALSE;
   if(!(locale=OpenLocale(NULL))) return FALSE;
   return TRUE;
}

static void Expungeaweblib(struct Library *libbase)
{  if(locale) CloseLocale(locale);
   if(AslBase) CloseLibrary(AslBase);
   if(GfxBase) CloseLibrary(GfxBase);
   if(IntuitionBase) CloseLibrary(IntuitionBase);
   if(UtilityBase) CloseLibrary(UtilityBase);
   if(LocaleBase) CloseLibrary(LocaleBase);
   if(DOSBase) CloseLibrary(DOSBase);
}

/*-----------------------------------------------------------------------*/

UBYTE *Jdupstr(UBYTE *str,long len,void *pool)
{  UBYTE *dup=NULL;
   if(str)
   {  if(len<0) len=strlen(str);
      if(dup=ALLOCTYPE(UBYTE,len+1,0,pool))
      {  strncpy(dup,str,len);
      }
   }
   return dup;
}

struct Jbuffer *Newjbuffer(void *pool)
{  struct Jbuffer *jb=ALLOCSTRUCT(Jbuffer,1,0,pool);
   if(jb)
   {  jb->pool=pool;
   }
   return jb;
}

void Freejbuffer(struct Jbuffer *jb)
{  if(jb)
   {  if(jb->buffer) FREE(jb->buffer);
      FREE(jb);
   }
}

void Addtojbuffer(struct Jbuffer *jb,UBYTE *text,long length)
{  if(length<0) length=strlen(text);
   if(jb->length+length+1>jb->size)
   {  long newsize=((jb->length+length+1024)/1024)*1024;
      UBYTE *newbuf=ALLOCTYPE(UBYTE,newsize,0,jb->pool);
      if(!newbuf) return;
      if(jb->size)
      {  memmove(newbuf,jb->buffer,jb->size);
         FREE(jb->buffer);
      }
      jb->buffer=newbuf;
      jb->size=newsize;
   }
   if(length)
   {  memmove(jb->buffer+jb->length,text,length);
      jb->length+=length;
   }
}

BOOL Calleractive(void)
{  struct IntuitionBase *ibase=(struct IntuitionBase *)IntuitionBase;
   ULONG lock=LockIBase(0);
   BOOL active=FALSE;
   if(ibase->ActiveWindow && ibase->ActiveWindow->UserPort
   && ibase->ActiveWindow->UserPort->mp_SigTask==FindTask(NULL))
   {  active=TRUE;
   }
   UnlockIBase(lock);
   return active;
}

static long __saveds __asm Idcmphook(register __a0 struct Hook *hook,
   register __a1 struct IntuiMessage *msg)
{  struct Jcontext *jc=hook->h_Data;
   if(msg->Class==IDCMP_CHANGEWINDOW)
   {  jc->feedback(jc);
   }
   return 0;
}

/* lnr<0 gives general requester. pos<0 gives runtime, pos>=0 gives parsing requester.
 * Returns TRUE when to ignore, FALSE when to stop.
 * But: lnr<0 && pos>0 gives loop warning requester;
 * returns TRUE when to stop, FALSE when to continue. */
BOOL Errorrequester(struct Jcontext *jc,long lnr,UBYTE *line,long pos,UBYTE *msg,UBYTE **args)
{  struct ClassLibrary *WindowBase=NULL,*LayoutBase=NULL,*ButtonBase=NULL,*LabelBase=NULL;
   BOOL ignore=FALSE;
   void *winobj,*buttonrow;
   ULONG sigmask,result;
   UBYTE lnrbuf[32],src[128],buf[128];
   UBYTE *p,*q;
   long len;
   BOOL done=FALSE;
   struct TextAttr ta={ 0 };
   struct Screen *screen;
   if(lnr>=0)
   {  sprintf(lnrbuf,"Line %d\n",lnr);
      if(pos>=0)
      {  if(line)
         {  for(p=line+pos-1;p>=line && *p!='\n' && *p!='\r';p--);
            pos-=(p+1-line);
            line=p+1;
            for(p=line;*p && *p!='\n' && *p!='\r';p++);
            len=p-line;
            if(pos<=30)
            {  q=line;
               if(len>60) len=60;
            }
            else
            {  if(len>60)
               {  if(len>pos+30)
                  {  q=line+pos-30;
                     pos=30;
                     len=60;
                  }
                  else
                  {  q=line+len-60;
                     pos-=len-60;
                     len=60;
                  }
               }
               else
               {  q=line;
               }
            }
            strncpy(src,q,len);
            p=src+len;
         }
         else
         {  p=src;
         }
         *p++='\n';
         for(;pos;pos--) *p++=' ';
         *p++='^';
         *p++='\0';
      }
      else if(line)
      {  strncpy(src,line,60);
      }
      else
      {  *src='\0';
      }
   }
   else
   {  *lnrbuf='\0';
      *src='\0';
   }
   vsprintf(buf,msg,(va_list)args);
   ta.ta_Name=((struct GfxBase *)GfxBase)->DefaultFont->ln_Name;
   ta.ta_YSize=((struct GfxBase *)GfxBase)->DefaultFont->tf_YSize;
   if(!(screen=LockPubScreen(jc->screenname)))
   {  screen=LockPubScreen(NULL);
   }
   if(screen
   && (WindowBase=(struct ClassLibrary *)OpenLibrary("window.class",OSNEED(0,44)))
   && (LayoutBase=(struct ClassLibrary *)OpenLibrary("gadgets/layout.gadget",OSNEED(0,44)))
   && (ButtonBase=(struct ClassLibrary *)OpenLibrary("gadgets/button.gadget",OSNEED(0,44)))
   && (LabelBase=(struct ClassLibrary *)OpenLibrary("images/label.image",OSNEED(0,44))))
   {  winobj=WindowObject,
         WA_Title,(lnr<0 && pos>0)?"JavaScript warning":"JavaScript error",
         WA_DepthGadget,TRUE,
         WA_DragBar,TRUE,
         WA_Activate,Calleractive(),
         WA_AutoAdjust,TRUE,
         WA_SimpleRefresh,TRUE,
         WA_PubScreen,screen,
         WINDOW_Position,WPOS_CENTERSCREEN,
         WINDOW_IDCMPHook,&idcmphook,
         WINDOW_IDCMPHookBits,IDCMP_CHANGEWINDOW,
         WINDOW_Layout,VLayoutObject,
            LAYOUT_DeferLayout,TRUE,
            StartMember,VLayoutObject,
               LAYOUT_SpaceOuter,TRUE,
               StartImage,LabelObject,
                  LABEL_Text,lnrbuf,
               EndImage,
               StartImage,LabelObject,
                  IA_Font,&ta,
                  LABEL_Text,src,
                  LABEL_Underscore,0,
               EndImage,
               StartImage,LabelObject,
                  LABEL_Text,buf,
                  LABEL_Underscore,0,
               EndImage,
            EndMember,
            StartMember,buttonrow=HLayoutObject,
               LAYOUT_BevelStyle,BVS_SBAR_VERT,
               LAYOUT_SpaceOuter,TRUE,
               LAYOUT_EvenSize,TRUE,
               StartMember,ButtonObject,
                  GA_ID,1,
                  GA_RelVerify,TRUE,
                  GA_Text,"_Ok",
               EndMember,
               CHILD_WeightedWidth,0,
               CHILD_NominalSize,TRUE,
            EndMember,
         End,
      EndWindow;
      if(winobj && lnr>=0 && !(jc->dflags&DEBF_DOPEN))
      {  SetAttrs(buttonrow,
            StartMember,ButtonObject,
               GA_ID,2,
               GA_RelVerify,TRUE,
               GA_Text,pos<0?"_Debug":"Ignore _All",
            EndMember,
            CHILD_WeightedWidth,0,
            CHILD_NominalSize,TRUE,
            TAG_END);
      }
      if(winobj && lnr<0 && pos>0)
      {  SetAttrs(buttonrow,
            StartMember,ButtonObject,
               GA_ID,2,
               GA_RelVerify,TRUE,
               GA_Text,"_Stop",
            EndMember,
            CHILD_WeightedWidth,0,
            CHILD_NominalSize,TRUE,
            TAG_END);
      }
      if(winobj && CA_OpenWindow(winobj))
      {  GetAttr(WINDOW_SigMask,winobj,&sigmask);
         while(!done)
         {  Wait(sigmask);
            while(!done && (result=CA_HandleInput(winobj,NULL))!=WMHI_LASTMSG)
            {  switch(result&WMHI_CLASSMASK)
               {  case WMHI_GADGETUP:
                     if(result&WMHI_GADGETMASK)
                     {  done=TRUE;
                        ignore=((result&WMHI_GADGETMASK)==2);
                     }
                     break;
               }
            }
         }
      }
      if(winobj) DisposeObject(winobj);
      /* Allow window refreshing after requester closes */
      jc->feedback(jc);
   }
   if(screen) UnlockPubScreen(NULL,screen);
   if(LabelBase) CloseLibrary((struct Library *)LabelBase);
   if(ButtonBase) CloseLibrary((struct Library *)ButtonBase);
   if(LayoutBase) CloseLibrary((struct Library *)LayoutBase);
   if(WindowBase) CloseLibrary((struct Library *)WindowBase);
   return ignore;
}

BOOL Feedback(struct Jcontext *jc)
{  static long counter=0;
   unsigned int clock[2]={ 0,0 };
   BOOL run=TRUE;
   if(jc->flags&EXF_STOP) run=FALSE;
   /* Don't get the time etc with _every_ call to avoid excessive overhead */
   if(run && ++counter>20)
   {  counter=0;
      timer(clock);
      if(run && jc->warntime && clock[0]>jc->warntime)
      {  run=!Errorrequester(jc,-1,NULL,1,"JavaScript runs for one minute",NULL);
         if(run)
         {  timer(clock);
            jc->warntime=clock[0]+60;
         }
      }
      if(run && jc->warnmem && AvailMem(0)<jc->warnmem)
      {  run=!Errorrequester(jc,-1,NULL,1,"JavaScript seems to be eating up all memory",NULL);
         if(run)
         {  jc->warnmem=AvailMem(0)/4;
         }
      }
      if(run && jc->feedback && clock[0]>jc->fbtime)
      {  run=jc->feedback(jc);
         jc->fbtime=clock[0]+1;
      }
   }
   return run;
}

/*-----------------------------------------------------------------------*/

__asm __saveds void *Newjcontext(register __a0 UBYTE *screenname)
{  struct Jcontext *jc=NULL;
   void *pool;
   /* New Jcontext is created in its own pool */
   if(pool=CreatePool(MEMF_PUBLIC|MEMF_CLEAR,PUDDLESIZE,TRESHSIZE))
   {  if(jc=ALLOCSTRUCT(Jcontext,1,0,pool))
      {  jc->pool=pool;
         NEWLIST(&jc->objects);
         Newexecute(jc);
         jc->screenname=screenname;
      }
   }
   if(!jc)
   {  if(pool) DeletePool(pool);
   }
   return jc;
}

__asm __saveds void Freejcontext(register __a0 struct Jcontext *jc)
{  if(jc)
   {  if(jc->pool)
      {  Freeexecute(jc);
         DeletePool(jc->pool);
         /* This deletes jc itself too because it was allocated in the pool */
      }
   }
}

__asm __saveds BOOL Runjprogram(register __a0 struct Jcontext *jc,
   register __a1 struct Jobject *fscope,
   register __a2 UBYTE *source,
   register __a3 struct Jobject *jthis,
   register __a4 struct Jobject **gwtab,
   register __d0 ULONG protkey,
   register __d1 ULONG userdata)
{  BOOL result=TRUE;
   ULONG olduserdata,oldprotkey,olddflags;
   long oldwarnmem;
   struct Value val={0};
   LIST(Jobject) temps;
   struct Jobject *jo,*jn;
   unsigned int clock[2]={ 0,0 };
   if(jc && source)
   {  idcmphook.h_Entry=(HOOKFUNC)Idcmphook;
      idcmphook.h_Data=jc;
      jc->flags&=~(JCF_ERROR|JCF_IGNORE|EXF_STOP);
      jc->generation++;
      jc->fscope=fscope;
      jc->warntime=0;
      jc->warnmem=0;
      Jcompile(jc,source);
      jc->linenr=0;
      if(!(jc->flags&JCF_ERROR))
      {  /* Remember existing temporary objects (see comment in jexe.c:Exedot()) */
         NEWLIST(&temps);
         for(jo=jc->objects.first;jo->next;jo=jn)
         {  jn=jo->next;
            if(jo->flags&OBJF_TEMP)
            {  REMOVE(jo);
               ADDTAIL(&temps,jo);
            }
         }
         olduserdata=jc->userdata;
         jc->userdata=userdata;
         oldprotkey=jc->protkey;
         jc->protkey=protkey;
         olddflags=jc->dflags;
         jc->dflags&=~DEBF_DOPEN;
         oldwarnmem=jc->warnmem;
         if((jc->dflags&DEBF_DEBUG) && (jc->dflags&DEBF_DBREAK))
         {  Startdebugger(jc);
         }
         if(jc->flags&EXF_WARNINGS)
         {  timer(clock);
            jc->warntime=clock[0]+60;
            jc->warnmem=AvailMem(0)/4;
         }
         Jexecute(jc,jthis,gwtab);
         if(jc->dflags&DEBF_DOPEN)
         {  Stopdebugger(jc);
         }
         jc->dflags=olddflags;
         Asgvalue(&val,jc->val);
         if(val.type==VTP_UNDEFINED) result=TRUE;
         else if(val.type==VTP_OBJECT && !val.ovalue) result=TRUE;
         else
         {  Toboolean(&val,jc);
            result=val.bvalue;
         }
         Clearvalue(&val);
         jc->warnmem=oldwarnmem;
         jc->userdata=olduserdata;
         jc->protkey=oldprotkey;
         while(jo=REMHEAD(&temps))
         {  ADDTAIL(&jc->objects,jo);
         }
      }
   }
   return result;
}

void __stdargs _XCEXIT(long x)
{
}

__asm __saveds void *Newjobject(
   register __a0 struct Jcontext *jc)
{  struct Jobject *jo;
   if(jo=Newobject(jc))
   {  Initconstruct(jc,jo,NULL);
   }
   return jo;
}

__asm __saveds void Disposejobject(
   register __a0 struct Jobject *jo)
{  struct Variable *var;
   /* In order to cope with reference loops, all references are cleared before
    * the object is unused.
    * Also, all possible references to external objects are cleared. */
   if(jo)
   {  for(var=jo->properties.first;var->next;var=var->next)
      {  if(var->val.type==VTP_OBJECT)
         {  Clearvalue(&var->val);
         }
         var->hookdata=NULL;
      }
      jo->internal=NULL;
      jo->dispose=NULL;
      jo->keepnr=0;
   }
}

__asm __saveds struct Jobject *AddjfunctionA(
   register __a0 struct Jcontext *jc,
   register __a1 struct Jobject *jo,
   register __a2 UBYTE *name,
   register __a3 void (*code)(void *),
   register __a4 UBYTE **args)
{  struct Jobject *f=NULL;
   struct Variable *prop;
   if(name)
   {  if((prop=Findproperty(jo,name))
      || (prop=Addproperty(jo,name)))
      {  if(f=InternalfunctionA(jc,name,code,args))
         {  if(f->function)
            {  f->function->fscope=jo;
            }
            Asgfunction(&prop->val,f,jo);
         }
      }
   }
   return f;
}

__asm __saveds struct Variable *Jfargument(
   register __a0 struct Jcontext *jc,
   register __d0 long n)
{  struct Jobject *jo;
   struct Variable *var;
   jo=jc->functions.first->arguments;
   var=Arrayelt(jo,n);
   return var;
}

__asm __saveds UBYTE *Jtostring(
   register __a0 struct Jcontext *jc,
   register __a1 struct Variable *jv)
{  Tostring(&jv->val,jc);
   return jv->val.svalue;
}

__asm __saveds void Jasgstring(
   register __a0 struct Jcontext *jc,
   register __a1 struct Variable *jv,
   register __a2 UBYTE *string)
{  Asgstring(jv?&jv->val:&jc->functions.first->retval,string?string:(UBYTE *)"",jc->pool);
}

__asm __saveds void Jasgobject(
   register __a0 struct Jcontext *jc,
   register __a1 struct Variable *jv,
   register __a2 struct Jobject *jo)
{  Asgobject(jv?&jv->val:&jc->functions.first->retval,jo);
}

__asm __saveds void Setjobject(
   register __a0 struct Jobject *jo,
   register __a1 Objhookfunc *hook,
   register __a2 void *internal,
   register __a3 void (*dispose)(void *))
{  jo->hook=hook;
   jo->internal=internal;
   jo->dispose=dispose;
}

__asm __saveds struct Variable *Jproperty(
   register __a0 struct Jcontext *jc,
   register __a1 struct Jobject *jo,
   register __a2 UBYTE *name)
{  struct Variable *var=NULL;
   if(!name) name="";
   if(!(var=Findproperty(jo,name)))
   {  var=Addproperty(jo,name);
   }
   return var;
}

__asm __saveds void Setjproperty(
   register __a0 struct Variable *jv,
   register __a1 Varhookfunc *hook,
   register __a2 void *hookdata)
{  if(hook==(Varhookfunc *)-1)
   {  jv->hook=Constantvhook;
   }
   else if(hook==(Varhookfunc *)-2)
   {  jv->flags|=VARF_SYNONYM;
      jv->hookdata=hookdata;
   }
   else
   {  jv->hook=hook;
      jv->hookdata=hookdata;
   }
}

__asm __saveds struct Jobject *Jthis(
   register __a0 struct Jcontext *jc)
{  return jc?jc->jthis:NULL;
}

__asm __saveds void *Jointernal(
   register __a0 struct Jobject *jo)
{  return jo?jo->internal:NULL;
}

__asm __saveds void Jasgboolean(
   register __a0 struct Jcontext *jc,
   register __a1 struct Variable *jv,
   register __d0 BOOL bvalue)
{  Asgboolean(jv?&jv->val:&jc->functions.first->retval,bvalue);
}

__asm __saveds BOOL Jtoboolean(
   register __a0 struct Jcontext *jc,
   register __a1 struct Variable *jv)
{  Toboolean(&jv->val,jc);
   return jv->val.bvalue;
}

__asm __saveds struct Jobject *Newjarray(
   register __a0 struct Jcontext *jc)
{  return Newarray(jc);
}

__asm __saveds struct Variable *Jnewarrayelt(
   register __a0 struct Jcontext *jc,
   register __a1 struct Jobject *jo)
{  return Addarrayelt(jc,jo);
}

__asm __saveds struct Jobject *Jtoobject(
   register __a0 struct Jcontext *jc,
   register __a1 struct Variable *jv)
{  Toobject(&jv->val,jc);
   return jv->val.ovalue;
}

__asm __saveds long Jtonumber(
   register __a0 struct Jcontext *jc,
   register __a1 struct Variable *jv)
{  long n;
   Tonumber(&jv->val,jc);
   if(jv->val.attr==VNA_VALID) n=(long)jv->val.nvalue;
   else n=0;
   return n;
}

__asm __saveds void Jasgnumber(
   register __a0 struct Jcontext *jc,
   register __a1 struct Variable *jv,
   register __d0 long nvalue)
{  Asgnumber(jv?&jv->val:&jc->functions.first->retval,VNA_VALID,(double)nvalue);
}

__asm __saveds BOOL Jisarray(
   register __a0 struct Jcontext *jc,
   register __a1 struct Jobject *jo)
{  return Isarray(jo);
}

__asm __saveds struct Jobject *Jfindarray(
   register __a0 struct Jcontext *jc,
   register __a1 struct Jobject *jo,
   register __a2 UBYTE *name)
{  struct Variable *var;
   struct Jobject *ja=NULL;
   if(var=Findproperty(jo,name))
   {  if(var->val.type==VTP_OBJECT && var->val.ovalue && Isarray(var->val.ovalue))
      {  ja=var->val.ovalue;
      }
   }
   return ja;
}

__asm __saveds void Jsetprototype(
   register __a0 struct Jcontext *jc,
   register __a1 struct Jobject *jo,
   register __a2 struct Jobject *proto)
{  struct Variable *var;
   if((var=Findproperty(jo,"prototype"))
   || (var=Addproperty(jo,"prototype")))
   {  if(!proto->constructor) proto->constructor=jo;
      proto->hook=Prototypeohook;
      Asgobject(&var->val,proto);
   }
}

__asm __saveds ULONG Jgetuserdata(
   register __a0 struct Jcontext *jc)
{  if(jc)
   {  return jc->userdata;
   }
   else
   {  return NULL;
   }
}

__asm __saveds BOOL Jisnumber(
   register __a0 struct Variable *jv)
{  return (BOOL)(jv && jv->val.type==VTP_NUMBER && jv->val.attr==VNA_VALID);
}

__asm __saveds void Clearjobject(
   register __a0 struct Jobject *jo,
   register __a1 UBYTE **except)
{  if(jo)
   {  Clearobject(jo,except);
   }
}

__asm __saveds void Freejobject(
   register __a0 struct Jobject *jo)
{
}

__asm __saveds void Jdumpobjects(
   register __a0 struct Jcontext *jc)
{  if(jc)
   {  Dumpobjects(jc);
   }
}

__asm __saveds struct Variable *Jgetreturnvalue(
   register __a0 struct Jcontext *jc)
{  struct Variable *var;
   if(jc && jc->result->val.type) var=jc->result;
   else var=NULL;
   return var;
}

__asm __saveds void Jpprotect(
   register __a0 struct Variable *var,
   register __d0 ULONG protkey)
{  if(var)
   {  var->protkey=protkey;
   }
}

__asm __saveds void Jcprotect(
   register __a0 struct Jcontext *jc,
   register __d0 ULONG protkey)
{  if(jc)
   {  jc->protkey=protkey;
   }
}

__asm __saveds UBYTE *Jpname(
   register __a0 struct Variable *var)
{  if(var) return var->name;
   else return NULL;
}

__asm __saveds void *Jdisposehook(
   register __a0 struct Jobject *jo)
{  void *hook=NULL;
   if(jo) hook=(void *)jo->dispose;
   return hook;
}

__asm __saveds void Jsetfeedback(
   register __a0 struct Jcontext *jc,
   register __a1 Jfeedback *jf)
{  if(jc)
   {  jc->feedback=jf;
   }
}

__asm __saveds void Jdebug(
   register __a0 struct Jcontext *jc,
   register __d0 BOOL debugon)
{  if(jc)
   {  if(debugon)
      {  jc->dflags|=DEBF_DEBUG|DEBF_DBREAK;
      }
      else
      {  jc->dflags&=~DEBF_DEBUG|DEBF_DBREAK;
      }
   }
}

__asm __saveds void Jerrors(
   register __a0 struct Jcontext *jc,
   register __d0 BOOL comperrors,
   register __d1 long runerrors,
   register __d2 BOOL watch)
{  if(jc)
   {  if(comperrors)
      {  jc->flags|=JCF_ERRORS;
      }
      else
      {  jc->flags&=~JCF_ERRORS;
      }
      jc->flags&=~(EXF_ERRORS|EXF_DONTSTOP);
      switch(runerrors)
      {  case JERRORS_CONTINUE:
            jc->flags|=EXF_DONTSTOP;
            break;
         case JERRORS_OFF:
            break;
         default:
            jc->flags|=EXF_ERRORS;
            break;
      }
      if(watch)
      {  jc->flags|=EXF_WARNINGS;
      }
      else
      {  jc->flags&=~EXF_WARNINGS;
      }
   }
}

__asm __saveds void Jkeepobject(
   register __a0 struct Jobject *jo,
   register __d0 BOOL used)
{  if(jo) Keepobject(jo,used);
}

__asm __saveds void Jgarbagecollect(
   register __a0 struct Jcontext *jc)
{  if(jc) Garbagecollect(jc);
}

__asm __saveds void Jsetlinenumber(
   register __a0 struct Jcontext *jc,
   register __d0 long linenr)
{  if(jc) jc->linenr=linenr;
}

__asm __saveds void Jsetobjasfunc(
   register __a0 struct Jobject *jo,
   register __d0 BOOL asfunc)
{  if(jo)
   {  if(asfunc)
      {  jo->flags|=OBJF_ASFUNCTION;
      }
      else
      {  jo->flags&=~OBJF_ASFUNCTION;
      }
   }
}

__asm __saveds void Jsetscreen(
   register __a0 struct Jcontext *jc,
   register __a1 UBYTE *screenname)
{  if(jc)
   {  jc->screenname=screenname;
   }
}

__asm __saveds void Jaddeventhandler(
   register __a0 struct Jcontext *jc,
   register __a1 struct Jobject *jo,
   register __a2 UBYTE *name,
   register __a3 UBYTE *source)
{  struct Variable *var;
   struct Jobject *jeventh;
   if(jc && jo && !Findproperty(jo,name))
   {  if(var=Addproperty(jo,name))
      {  if(source)
         {  jc->generation++;
            if(jeventh=Jcompiletofunction(jc,source,name))
            {  Asgfunction(&var->val,jeventh,jo);
            }
            else
            {  Asgstring(&var->val,source,jc->pool);
            }
         }
         else
         {  Asgobject(&var->val,NULL);
         }
      }
   }
}
