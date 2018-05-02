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

/* hotlist.c - AWeb hotlist manager */

#include "aweb.h"
#include "url.h"
#include "file.h"
#include "application.h"
#include "window.h"
#include "task.h"
#include <clib/utility_protos.h>

#define HOTLISTNAME "AWebPath:aweb.hotlist"
static UBYTE *hotlistname;

#include "hotlist.h"

static LIST(Hotbase) hotbase;
static LIST(Hotitem) hotlist;

/* Max size of URL. An I/O buffer will be allocated to allow this plus some extra. */
#define MAXURLSIZE   4096

static struct SignalSemaphore hotsema;

static BOOL hotchanged;

/* m is the maintenance window, v is the view window */
static struct Hotwindow *hotwindowm=NULL,*hotwindowv=NULL;

enum LIB_ENTRIES
{  LIBENTRY_VIEWER=0,LIBENTRY_MANAGER,
};

/*-----------------------------------------------------------------------*/

/* Find the base for this entry, create a new one of not found */
static struct Hotbase *Findbase(UBYTE *title,UBYTE *url)
{  struct Hotbase *hb,*newhb;
   long c,l;
   if(!title || !url) return NULL;
   for(hb=hotbase.first;hb->next;hb=hb->next)
   {  c=stricmp(hb->title,title);
      if(c==0 && STREQUAL(hb->url,url))
      {  return hb;
      }
      if(c>0) break;
   }
   /* If not found, create a new one and insert before (hb) */
   hb=hb->prev;
   if(newhb=ALLOCSTRUCT(Hotbase,1,MEMF_CLEAR|MEMF_PUBLIC))
   {  newhb->nodetype=HNT_BASE;
      l=strlen(url);
      if(l>MAXURLSIZE) l=MAXURLSIZE;
      if((newhb->title=Dupstr(title,-1))
      && (newhb->url=Dupstr(url,l)))
      {  INSERT(&hotbase,newhb,hb);
         return newhb;
      }
      if(newhb->url) FREE(newhb->url);
      if(newhb->title) FREE(newhb->title);
      FREE(newhb);
   }
   return NULL;
}

static void Readhotgroup(struct Hotitem *owner,void *hlist,long fh,
   UBYTE *buffer)
{  
#ifndef DEMOVERSION
   struct Hotitem *hi;
   struct Hotbase *hb;
   UBYTE *url;
   long l;
   BOOL makeitem;
   ULONG date=0;
   for(;;)
   {  if(!FGets(fh,buffer,MAXURLSIZE+1)) return;
      l=strlen(buffer);
      if(buffer[l-1]=='\n') buffer[l-1]='\0';
      if(STRIEQUAL(buffer,"@ENDGROUP")) return;
      hb=NULL;
      makeitem=TRUE;
      if(STRNIEQUAL(buffer,"@DATE ",6))
      {  sscanf(buffer+6,"%x",&date);
      }
      else
      {  if(buffer[0]!='@')
         {  url=Dupstr(buffer,-1);
            if(FGets(fh,buffer,MAXURLSIZE+1))
            {  l=strlen(buffer);
               if(buffer[l-1]=='\n') buffer[l-1]='\0';
               hb=Findbase(buffer,url);
               if(hb) hb->date=date;
               date=0;
            }
            if(url) FREE(url);
            if(!owner) makeitem=FALSE;
         }
         if(makeitem && (hi=ALLOCSTRUCT(Hotitem,1,MEMF_PUBLIC|MEMF_CLEAR)))
         {  hi->nodetype=HNT_ITEM;
            NEWLIST(&hi->subitems);
            if(STRNIEQUAL(buffer,"@GROUP_HIDE ",12))
            {  hi->type=HITEM_HGROUP;
               hi->name=Dupstr(buffer+12,-1);
               Readhotgroup(hi,&hi->subitems,fh,buffer);
            }
            else if(STRNIEQUAL(buffer,"@GROUP ",7))
            {  hi->type=HITEM_GROUP;
               hi->name=Dupstr(buffer+7,-1);
               Readhotgroup(hi,&hi->subitems,fh,buffer);
            }
            else if(hb)
            {  hi->type=HITEM_ENTRY;
               hi->base=hb;
               hb->used++;
            }
            hi->owner=owner;
            ADDTAIL(hlist,hi);
            date=0;
         }
      }
   }
#endif
}

static void Readawebhotlist(void)
{  
#ifndef DEMOVERSION
   long fh;
   UBYTE *iobuffer;
   if(iobuffer=ALLOCTYPE(UBYTE,MAXURLSIZE+512,MEMF_PUBLIC))
   {  if(fh=Open(hotlistname,MODE_OLDFILE))
      {  if(FGets(fh,iobuffer,MAXURLSIZE+1) && iobuffer[0]=='@')
         {  Readhotgroup(NULL,&hotlist,fh,iobuffer);
         }
         Close(fh);
      }
      FREE(iobuffer);
   }
#endif
}

static void Savehotgroup(LIST(Hotitem) *hlist,long fh,UBYTE *buffer)
{  
#ifndef DEMOVERSION
   struct Hotitem *hi;
   for(hi=hlist->first;hi->next;hi=hi->next)
   {  if(hi->type==HITEM_ENTRY && hi->base)
      {  if(hi->base->date)
         {  sprintf(buffer,"@DATE %012x\n",hi->base->date);
            Write(fh,buffer,strlen(buffer));
         }
         strcpy(buffer,hi->base->url);
         strcat(buffer,"\n");
         Write(fh,buffer,strlen(buffer));
         strcpy(buffer,hi->base->title);
         strcat(buffer,"\n");
         Write(fh,buffer,strlen(buffer));
      }
      else
      {  if(hi->type==HITEM_HGROUP) strcpy(buffer,"@GROUP_HIDE ");
         else strcpy(buffer,"@GROUP ");
         strcat(buffer,hi->name);
         strcat(buffer,"\n");
         Write(fh,buffer,strlen(buffer));
         Savehotgroup(&hi->subitems,fh,buffer);
         strcpy(buffer,"@ENDGROUP\n");
         Write(fh,buffer,strlen(buffer));
      }
   }
#endif
}

static void Saveawebhotlist(void)
{  
#ifndef DEMOVERSION
   long fh;
   struct Hotbase *hb;
   UBYTE *iobuffer;
   if(iobuffer=ALLOCTYPE(UBYTE,MAXURLSIZE+512,MEMF_PUBLIC))
   {  if(fh=Open(hotlistname,MODE_NEWFILE))
      {  strcpy(iobuffer,"@AWeb hotlist\n");
         Write(fh,iobuffer,strlen(iobuffer));
         Savehotgroup(&hotlist,fh,iobuffer);
         for(hb=hotbase.first;hb->next;hb=hb->next)
         {  if(!hb->used)
            {  if(hb->date)
               {  sprintf(iobuffer,"@DATE %012x\n",hb->date);
                  Write(fh,iobuffer,strlen(iobuffer));
               }
               strcpy(iobuffer,hb->url);
               strcat(iobuffer,"\n");
               Write(fh,iobuffer,strlen(iobuffer));
               strcpy(iobuffer,hb->title);
               strcat(iobuffer,"\n");
               Write(fh,iobuffer,strlen(iobuffer));
            }
         }
         Close(fh);
      }
      FREE(iobuffer);
   }
#endif
}

static void Disposehotgroup(void *hlist)
{  struct Hotitem *hi;
   while(hi=REMHEAD(hlist))
   {  if(hi->name) FREE(hi->name);
      Disposehotgroup(&hi->subitems);
      FREE(hi);
   }
}

static void Disposehotlist(void)
{  struct Hotbase *hb;
   Disposehotgroup(&hotlist);
   while(hb=REMHEAD(&hotbase))
   {  if(hb->title) FREE(hb->title);
      if(hb->url) FREE(hb->url);
      FREE(hb);
   }
}

/* Get a group in this stem variable. */
static void Gethotlistgroup(LIST (Hotitem) *list,UBYTE *owner,
   struct Arexxcmd *ac,UBYTE *stem,long *stemnr,BOOL groupsonly)
{  UBYTE buf[12];
   struct Hotitem *hi;
   for(hi=list->first;hi->next;hi=hi->next)
   {  if(hi->type==HITEM_ENTRY)
      {  if(!groupsonly)
         {  (*stemnr)++;
            Setstemvar(ac,stem,*stemnr,"GROUP","0");
            Setstemvar(ac,stem,*stemnr,"TITLE",hi->base->title);
            Setstemvar(ac,stem,*stemnr,"URL",hi->base->url);
            Setstemvar(ac,stem,*stemnr,"OWNER",owner);
         }
      }
      else
      {  (*stemnr)++;
         if(!groupsonly)
         {  Setstemvar(ac,stem,*stemnr,"GROUP","1");
         }
         Setstemvar(ac,stem,*stemnr,"TITLE",hi->name);
         Setstemvar(ac,stem,*stemnr,"OWNER",owner);
         sprintf(buf,"%d",*stemnr);
         Gethotlistgroup(&hi->subitems,buf,ac,stem,stemnr,groupsonly);
      }
   }
}

static void Buildfromstem(struct Arexxcmd *ac,UBYTE *stem)
{  long stemnr,maxnr=0;
   UBYTE *value,*title,*url;
   BOOL group;
   long owner;
   struct Hotbase *hb;
   struct Hotitem *hi;
   struct Hotitem **grouptab;
   if(value=Getstemvar(ac,stem,0,NULL))
   {  sscanf(value," %ld",&maxnr);
   }
   if(maxnr)
   {  if(grouptab=ALLOCTYPE(struct Hotitem *,maxnr+1,MEMF_CLEAR))
      {  for(stemnr=1;stemnr<=maxnr;stemnr++)
         {  if(value=Getstemvar(ac,stem,stemnr,"GROUP"))
            {  sscanf(value," %hd",&group);
            }
            else
            {  group=FALSE;
            }
            if(value=Getstemvar(ac,stem,stemnr,"OWNER"))
            {  sscanf(value," %ld",&owner);
               if(owner<1 || owner>maxnr || !grouptab[owner]) owner=0;
            }
            else
            {  owner=0;
            }
            title=Getstemvar(ac,stem,stemnr,"TITLE");
            if(!group)
            {  url=Getstemvar(ac,stem,stemnr,"URL");
            }
            if(title && (group || url))
            {  if(!group)
               {  if(!(hb=Findbase(title,url))) return;
               }
               if(group || owner)
               {  if(!(hi=ALLOCSTRUCT(Hotitem,1,MEMF_PUBLIC|MEMF_CLEAR))) return;
                  NEWLIST(&hi->subitems);
                  if(owner)
                  {  hi->owner=grouptab[owner];
                     ADDTAIL(&hi->owner->subitems,hi);
                  }
                  else
                  {  ADDTAIL(&hotlist,hi);
                  }
                  if(group)
                  {  hi->type=HITEM_GROUP;
                     if(!(hi->name=Dupstr(title,-1)))
                     {  FREE(hi);
                        return;
                     }
                     grouptab[stemnr]=hi;
                  }
                  else
                  {  hi->base=hb;
                     hb->used++;
                  }
               }
            }
         }
         FREE(grouptab);
      }
   }
}

/*-----------------------------------------------------------------------*/

/* Open aweblib and start task */
static BOOL Starttask(struct Hotwindow *how)
{  if(how->libbase=Openaweblib("AWebPath:aweblib/hotlist.aweblib"))
   {  how->task=Anewobject(AOTP_TASK,
         AOTSK_Entry,AWEBLIBENTRY(how->libbase,how->libentry),
         AOTSK_Name,"AWeb hotlist",
         AOTSK_Userdata,how,
         AOBJ_Target,how,
         TAG_END);
      if(how->task)
      {  Asetattrs(how->task,AOTSK_Start,TRUE,TAG_END);
      }
      else
      {  CloseLibrary(how->libbase);
         how->libbase=NULL;
      }
   }
   return BOOLVAL(how->task);
}

/* Dispose task and close aweblib */
static void Stoptask(struct Hotwindow *how)
{  if(how->libbase)
   {  if(how->task)
      {  Adisposeobject(how->task);
         how->task=NULL;
      }
      CloseLibrary(how->libbase);
      how->libbase=NULL;
   }
}

/*-----------------------------------------------------------------------*/

static long Updatehotwindow(struct Hotwindow *how,struct Amset *ams)
{  struct TagItem *tag,*tstate=ams->tags;
   BOOL close=FALSE,dim=FALSE;
   void *url,*win;
   while(tag=NextTagItem(&tstate))
   {  switch(tag->ti_Tag)
      {  case AOHOT_Close:
            if(tag->ti_Data) close=TRUE;
            break;
         case AOHOT_Dimx:
            how->x=tag->ti_Data;
            dim=TRUE;
            break;
         case AOHOT_Dimy:
            how->y=tag->ti_Data;
            break;
         case AOHOT_Dimw:
            how->w=tag->ti_Data;
            break;
         case AOHOT_Dimh:
            how->h=tag->ti_Data;
            break;
         case AOHOT_Follow:
            if(tag->ti_Data)
            {  if(url=Findurl("",(UBYTE *)tag->ti_Data,0))
               {  if(!(win=Findwindow(how->windowkey)))
                  {  win=Activewindow();
                  }
                  if(win)
                  {  Inputwindoc(win,url,NULL,0);
                  }
               }
            }
            break;
         case AOHOT_Changed:
            hotchanged|=tag->ti_Data;
#ifndef DEMOVERSION
            if(hotwindowv && hotwindowv->task)
            {  Asetattrsasync(hotwindowv->task,AOHOT_Addentry,TRUE,TAG_END);
            }
#endif
            break;
         case AOHOT_Save:
            Savehotlist();
            break;
         case AOHOT_Restore:
            Restorehotlist();
            break;
         case AOHOT_Manager:
            Hotlistmanager(how->windowkey);
            break;
         case AOHOT_Vchanged:
            hotchanged|=tag->ti_Data;
            break;
      }
   }
   if(dim)
   {  if(how->libentry==LIBENTRY_VIEWER)
      {  prefs.hotx=how->x;
         prefs.hoty=how->y;
         prefs.hotw=how->w;
         prefs.hoth=how->h;
      }
      else
      {  prefs.homx=how->x;
         prefs.homy=how->y;
         prefs.homw=how->w;
         prefs.homh=how->h;
      }
   }
   if(close && !(how->flags&HOTF_BREAKING))
   {  Stoptask(how);
   }
   return 0;
}

static long Sethotwindow(struct Hotwindow *how,struct Amset *ams)
{  struct TagItem *tag,*tstate=ams->tags;
   while(tag=NextTagItem(&tstate))
   {  switch(tag->ti_Tag)
      {  case AOAPP_Screenvalid:
            if(!tag->ti_Data)
            {  if(how->task)
               {  how->flags|=HOTF_BREAKING|HOTF_WASOPEN;
                  Stoptask(how);
                  how->flags&=~HOTF_BREAKING;
               }
            }
            else
            {  if((how->flags&HOTF_WASOPEN) && !how->task)
               {  how->screenname=(UBYTE *)Agetattr(Aweb(),AOAPP_Screenname);
                  Starttask(how);
               }
            }
            break;
      }
   }
   return 0;
}

static struct Hotwindow *Newhotwindow(struct Amset *ams)
{  struct Hotwindow *how;
   if(how=Allocobject(AOTP_HOTLIST,sizeof(struct Hotwindow),ams))
   {  Aaddchild(Aweb(),how,AOREL_APP_USE_SCREEN);
      how->windowkey=GetTagData(AOHOT_Windowkey,0,ams->tags);
      how->libentry=GetTagData(AOHOT_Libentry,0,ams->tags);
      if(Agetattr(Aweb(),AOAPP_Screenvalid))
      {  how->hotsema=&hotsema;
         how->hotbase=&hotbase;
         how->hotlist=&hotlist;
         if(how->libentry==LIBENTRY_VIEWER)
         {  how->x=prefs.hotx;
            how->y=prefs.hoty;
            how->w=prefs.hotw;
            how->h=prefs.hoth;
            how->autoclose=prefs.hlautoclose;
         }
         else
         {  how->x=prefs.homx;
            how->y=prefs.homy;
            how->w=prefs.homw;
            how->h=prefs.homh;
         }
         how->screenname=(UBYTE *)Agetattr(Aweb(),AOAPP_Screenname);
         if(!Starttask(how))
         {  Adisposeobject(how);
            how=NULL;
         }
      }
   }
   return how;
}

static void Disposehotwindow(struct Hotwindow *how)
{  if(how->task)
   {  how->flags|=HOTF_BREAKING;
      Stoptask(how);
   }
   Aremchild(Aweb(),how,AOREL_APP_USE_SCREEN);
   Amethodas(AOTP_OBJECT,how,AOM_DISPOSE);
}

static void Deinstallhotwindow(void)
{  if(hotwindowv) Adisposeobject(hotwindowv);
   if(hotwindowm) Adisposeobject(hotwindowm);
   if(hotchanged) Saveawebhotlist();
   if(hotbase.first) Disposehotlist();
   if(hotlistname) FREE(hotlistname);
}

static long Dispatch(struct Hotwindow *how,struct Amessage *amsg)
{  long result=0;
   switch(amsg->method)
   {  case AOM_NEW:
         result=(long)Newhotwindow((struct Amset *)amsg);
         break;
      case AOM_SET:
         result=Sethotwindow(how,(struct Amset *)amsg);
         break;
      case AOM_UPDATE:
         result=Updatehotwindow(how,(struct Amset *)amsg);
         break;
      case AOM_DISPOSE:
         Disposehotwindow(how);
         break;
      case AOM_DEINSTALL:
         Deinstallhotwindow();
         break;
   }
   return result;
}

/*-----------------------------------------------------------------------*/

BOOL Installhotlist(void)
{  InitSemaphore(&hotsema);
   NEWLIST(&hotbase);
   NEWLIST(&hotlist);
   if(!Amethod(NULL,AOM_INSTALL,AOTP_HOTLIST,Dispatch)) return FALSE;
   if(!hotlistname)
   {  if(!(hotlistname=Dupstr(HOTLISTNAME,-1))) return FALSE;
   }
   Readawebhotlist();
   return TRUE;
}

void Hotlistviewer(ULONG windowkey)
{  if(hotwindowv && hotwindowv->task)
   {  Asetattrsasync(hotwindowv->task,
         AOHOT_Tofront,TRUE,
         TAG_END);
   }
   else
   {  if(hotwindowv) Adisposeobject(hotwindowv);
      hotwindowv=Anewobject(AOTP_HOTLIST,
         AOHOT_Libentry,LIBENTRY_VIEWER,
         AOHOT_Windowkey,windowkey,
         TAG_END);
   }
}

void Hotlistmanager(ULONG windowkey)
{  if(hotwindowm && hotwindowm->task)
   {  Asetattrsasync(hotwindowm->task,
         AOHOT_Tofront,TRUE,
         TAG_END);
   }
   else
   {  if(hotwindowm) Adisposeobject(hotwindowm);
      hotwindowm=Anewobject(AOTP_HOTLIST,
         AOHOT_Libentry,LIBENTRY_MANAGER,
         AOHOT_Windowkey,windowkey,
         TAG_END);
   }
}

void Closehotlistviewer(void)
{  if(hotwindowv)
   {  Adisposeobject(hotwindowv);
      hotwindowv=NULL;
   }
}

void Closehotlistmanager(void)
{  if(hotwindowm)
   {  Adisposeobject(hotwindowm);
      hotwindowm=NULL;
   }
}

static void Addhtmltext(UBYTE *line,void *tf)
{  UBYTE *p,*q;
   p=line;
   for(;;)
   {  for(q=p;*q && *q!='<' && *q!='&';q++);
      if(q>p)
      {  Asetattrs(tf,
            AOFIL_Data,p,
            AOFIL_Datalength,q-p,
            TAG_END);
      }
      if(!*q) break;
      if(*q=='<') Asetattrs(tf,AOFIL_Stringdata,"&lt;",TAG_END);
      else Asetattrs(tf,AOFIL_Stringdata,"&amp;",TAG_END);
      p=q+1;
   }
}

static void Addhtml(LIST(Hotitem) *list,void *tf)
{  struct Hotitem *hi;
   for(hi=list->first;hi->next;hi=hi->next)
   {  Asetattrs(tf,AOFIL_Stringdata,"<li>",TAG_END);
      if(hi->type==HITEM_ENTRY && hi->base)
      {  Asetattrs(tf,
            AOFIL_Stringdata,"<a href=\"",
            AOFIL_Stringdata,hi->base->url,
            AOFIL_Stringdata,"\">",
            TAG_END);
         Addhtmltext(hi->base->title,tf);
         Asetattrs(tf,AOFIL_Stringdata,"</a>\n",TAG_END);
      }
      else
      {  Asetattrs(tf,AOFIL_Stringdata,"<strong>",TAG_END);
         Addhtmltext(hi->name,tf);
         Asetattrs(tf,AOFIL_Stringdata,"</strong><ul>\n",TAG_END);
         Addhtml(&hi->subitems,tf);
         Asetattrs(tf,AOFIL_Stringdata,"\n</ul>\n",TAG_END);
      }
   }
}

void Buildhtmlhotlist(void *tf)
{  UBYTE *title=AWEBSTR(MSG_AWEB_HOTLISTTITLE);
   struct Hotbase *hb;
   ObtainSemaphore(&hotsema);
   Asetattrs(tf,
      AOFIL_Stringdata,"<html><head><title>AWeb ",
      AOFIL_Stringdata,title,
      AOFIL_Stringdata,"</title></head><body><h1>AWeb ",
      AOFIL_Stringdata,title,
      AOFIL_Stringdata,"</h1>\n",
      AOFIL_Stringdata,"<ul>\n",
      TAG_END);
   Addhtml(&hotlist,tf);
   for(hb=hotbase.first;hb->next;hb=hb->next)
   {  if(!hb->used)
      {  Asetattrs(tf,
            AOFIL_Stringdata,"<li>",
            AOFIL_Stringdata,"<a href=\"",
            AOFIL_Stringdata,hb->url,
            AOFIL_Stringdata,"\">",
            TAG_END);
         Addhtmltext(hb->title,tf);
         Asetattrs(tf,AOFIL_Stringdata,"</a>\n",TAG_END);
      }
   }
   Asetattrs(tf,
      AOFIL_Stringdata,"</ul></body></html>\n",
      TAG_END);
   ReleaseSemaphore(&hotsema);
}

void Addtohotlist(void *url,UBYTE *title,UBYTE *group)
{  UBYTE *p,*q,*urlp,*path;
   long len;
   struct Hotbase *hb;
   struct Hotitem *hi,*hg;
   LIST(Hotitem) *hl;
   ULONG tag=AOHOT_Addentry;
   if(url)
   {  urlp=(UBYTE *)Agetattr(url,AOURL_Url);
      ObtainSemaphore(&hotsema);
      if(title && *title) p=title;
      else p=urlp;
      len=strlen(p);
      p=Dupstr(p,MIN(len,128));
      hb=Findbase(p,urlp);
      if(hb) hb->date=Today();
      if(p) FREE(p);
      hotchanged=TRUE;
      if(group
      && (path=Dupstr(group,-1)))
      {  /* Follow path through groups and subgroups of hotlist */
         hl=&hotlist;
         hg=NULL;
         p=path;
         while(*p)
         {  if(q=strchr(p,'\n')) *q++='\0';
            for(hi=hl->first;hi->next;hi=hi->next)
            {  if(hi->type>=HITEM_GROUP && hi->name
               && STRIEQUAL(hi->name,p))
               {  hg=hi;
                  hl=&hi->subitems;
                  if(q) p=q;
                  else p=p+strlen(p);
                  break;
               }
            }
            if(!hi->next)  /* Not found */
            {  break;
            }
         }
         if(hg && !*p)           /* Path found upto last segment, ok */
         {  /* See if this hotbase already is included in this group */
            for(hi=hg->subitems.first;hi->next;hi=hi->next)
            {  if(hi->base==hb) break;
            }
            if(!hi->next)  /* Not found, add new entry */
            {  if(hi=ALLOCSTRUCT(Hotitem,1,MEMF_CLEAR|MEMF_PUBLIC))
               {  hi->nodetype=HNT_ITEM;
                  hi->owner=hg;
                  hi->type=HITEM_ENTRY;
                  hi->base=hb;
                  NEWLIST(&hi->subitems);
                  ADDTAIL(&hg->subitems,hi);
                  hb->used++;
                  tag=AOHOT_Newlist;
               }
            }
         }
         FREE(path);
      }
      ReleaseSemaphore(&hotsema);
      if(hotwindowv && hotwindowv->task)
      {  Asetattrsasync(hotwindowv->task,tag,TRUE,TAG_END);
      }
      if(hotwindowm && hotwindowm->task)
      {  Asetattrsasync(hotwindowm->task,tag,TRUE,TAG_END);
      }
      Savehotlist();
   }
}

void Sethotlistname(UBYTE *name)
{  UBYTE *newname=Dupstr(name,-1);
   if(newname)
   {  if(hotlistname) FREE(hotlistname);
      hotlistname=newname;
   }
}

void Savehotlist(void)
{  ObtainSemaphore(&hotsema);
   if(hotchanged) Saveawebhotlist();
   hotchanged=FALSE;
   ReleaseSemaphore(&hotsema);
}

void Restorehotlist(void)
{  if(hotwindowv && hotwindowv->task)
   {  Asetattrssync(hotwindowv->task,AOHOT_Dispose,TRUE,TAG_END);
   }
   if(hotwindowm && hotwindowm->task)
   {  Asetattrssync(hotwindowm->task,AOHOT_Dispose,TRUE,TAG_END);
   }
   ObtainSemaphore(&hotsema);
   Disposehotlist();
   Readawebhotlist();
   hotchanged=FALSE;
   ReleaseSemaphore(&hotsema);
   if(hotwindowv && hotwindowv->task)
   {  Asetattrsasync(hotwindowv->task,AOHOT_Newlist,TRUE,TAG_END);
   }
   if(hotwindowm && hotwindowm->task)
   {  Asetattrsasync(hotwindowm->task,AOHOT_Newlist,TRUE,TAG_END);
   }
}

void Gethotlistcontents(struct Arexxcmd *ac,UBYTE *stem,BOOL groupsonly)
{  long stemnr=0;
   UBYTE buf[12];
   struct Hotbase *hb;
   Gethotlistgroup(&hotlist,"0",ac,stem,&stemnr,groupsonly);
   if(!groupsonly)
   {  for(hb=hotbase.first;hb->next;hb=hb->next)
      {  if(!hb->used)
         {  stemnr++;
            Setstemvar(ac,stem,stemnr,"GROUP","0");
            Setstemvar(ac,stem,stemnr,"TITLE",hb->title);
            Setstemvar(ac,stem,stemnr,"URL",hb->url);
            Setstemvar(ac,stem,stemnr,"OWNER","0");
         }
      }
   }
   sprintf(buf,"%d",stemnr);
   Setstemvar(ac,stem,0,NULL,buf);
}

void Sethotlistcontents(struct Arexxcmd *ac,UBYTE *stem)
{  if(hotwindowv && hotwindowv->task)
   {  Asetattrssync(hotwindowv->task,AOHOT_Dispose,TRUE,TAG_END);
   }
   if(hotwindowm && hotwindowm->task)
   {  Asetattrssync(hotwindowm->task,AOHOT_Dispose,TRUE,TAG_END);
   }
   ObtainSemaphore(&hotsema);
   Disposehotlist();
   Buildfromstem(ac,stem);
   hotchanged=TRUE;
   ReleaseSemaphore(&hotsema);
   if(hotwindowv && hotwindowv->task)
   {  Asetattrsasync(hotwindowv->task,AOHOT_Newlist,TRUE,TAG_END);
   }
   if(hotwindowm && hotwindowm->task)
   {  Asetattrsasync(hotwindowm->task,AOHOT_Newlist,TRUE,TAG_END);
   }
}

BOOL Isopenhotlistviewer(void)
{  return (BOOL)(hotwindowv && hotwindowv->task);
}

BOOL Isopenhotlistmanager(void)
{  return (BOOL)(hotwindowm && hotwindowm->task);
}