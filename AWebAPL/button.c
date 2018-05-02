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

/* button.c - AWeb html document button element object */

#include "aweb.h"
#include "button.h"
#include "form.h"
#include "body.h"
#include "application.h"
#include "url.h"
#include "window.h"
#include "jslib.h"
#include <classact.h>
#include <intuition/imageclass.h>
#include <clib/graphics_protos.h>
#include <clib/intuition_protos.h>
#include <clib/utility_protos.h>

/*------------------------------------------------------------------------*/

struct Button
{  struct Field field;
   void *pool;
   USHORT type;
   void *capens;
   void *frame;
   void *parent;
   USHORT flags;
   UBYTE *onclick;
   UBYTE *onfocus;
   UBYTE *onblur;
   void *body;             /* For custom label */
   long minw,maxw;
};

#define BUTF_SELECTED   0x0001   /* Button is in selected state */
#define BUTF_REMEASURE  0x0002   /* Re-measure even if width is known */
#define BUTF_COMPLETE   0x0004   /* Custom label is complete */

static void *bevel;
static long bevelw,bevelh;

/*------------------------------------------------------------------------*/

/* Perform the actions for clicking the button */
static void Clickbutton(struct Button *but,BOOL js)
{  switch(but->type)
   {  case BUTTP_SUBMIT:
         if(!js || Runjavascriptwith(but->cframe,awebonclick,&but->jobject,but->form))
         {  Amethod(but->form,AFO_SUBMIT,but,0,0);
         }
         break;
      case BUTTP_RESET:
         Amethod(but->form,AFO_RESET);
         if(js) Runjavascriptwith(but->cframe,awebonclick,&but->jobject,but->form);
         break;
      case BUTTP_BUTTON:
         if(js) Runjavascriptwith(but->cframe,awebonclick,&but->jobject,but->form);
         break;
   }
}

/*------------------------------------------------------------------------*/

/* Javascript methods */
static void Methodclick(struct Jcontext *jc)
{  struct Button *but=Jointernal(Jthis(jc));
   if(but) Clickbutton(but,FALSE);
}

static void Methodfocus(struct Jcontext *jc)
{
}

static void Methodblur(struct Jcontext *jc)
{
}

static void Methodtostring(struct Jcontext *jc)
{  struct Button *but=Jointernal(Jthis(jc));
   struct Buffer buf={0};
   UBYTE *p;
   if(but)
   {  Addtagstr(&buf,"<input",ATSF_NONE,0);
      switch(but->type)
      {  case BUTTP_SUBMIT:p="submit";break;
         case BUTTP_RESET: p="reset";break;
         default:          p="button";break;
      }
      Addtagstr(&buf,"type",ATSF_STRING,p);
      if(but->name) Addtagstr(&buf,"name",ATSF_STRING,but->name);
      if(but->value) Addtagstr(&buf,"value",ATSF_STRING,but->value);
      Addtobuffer(&buf,">",2);
      Jasgstring(jc,NULL,buf.buffer);
      Freebuffer(&buf);
   }
}

static BOOL Propertyvalue(struct Varhookdata *vd)
{  BOOL result=FALSE;
   struct Button *but=vd->hookdata;
   UBYTE *p;
   if(but)
   {  switch(vd->code)
      {  case VHC_SET:
            if(p=Dupstr(Jtostring(vd->jc,vd->value),-1))
            {  if(but->value) FREE(but->value);
               but->value=p;
               Arender(but,NULL,0,0,AMRMAX,AMRMAX,0,NULL);
            }
            result=TRUE;
            break;
         case VHC_GET:
            Jasgstring(vd->jc,vd->value,but->value);
            result=TRUE;
            break;
      }
   }
   return result;
}

/*------------------------------------------------------------------------*/

static long Measurebutton(struct Button *but,struct Ammeasure *amm)
{  /* Only re-measure if we haven't got a width yet. Because the text
    * can change under JS, a re-measure shouldn't resize the button.
    * There is no other way the size can change anyway. */
   if(!but->aow || (but->flags&BUTF_REMEASURE))
   {  if(but->body)
      {  if(but->flags&BUTF_COMPLETE)
         {  struct Ammresult ammr={0};
            Ameasure(but->body,amm->width,amm->height,0,0,amm->text,&ammr);
            but->aow=ammr.width;
            but->minw=ammr.minwidth;
            but->maxw=ammr.width;
            if(amm->ammr)
            {  amm->ammr->width=ammr.width;
               if(but->eltflags&(ELTF_NOBR|ELTF_PREFORMAT))
               {  amm->ammr->minwidth=amm->addwidth+ammr.minwidth;
                  amm->ammr->addwidth=amm->ammr->minwidth;
               }
               else
               {  amm->ammr->minwidth=ammr.minwidth;
               }
            }
            but->flags&=~BUTF_REMEASURE;
         }
      }
      else
      {  SetFont(mrp,(struct TextFont *)Agetattr(Aweb(),AOAPP_Screenfont));
         SetSoftStyle(mrp,0,0x0f);
         but->aow=Textlength(mrp,but->value,strlen(but->value))+2*bevelw+8;
         but->aoh=mrp->TxHeight+2*bevelh+2;
         but->flags&=~BUTF_REMEASURE;
         AmethodasA(AOTP_FIELD,but,amm);
      }
   }
   else
   {  AmethodasA(AOTP_FIELD,but,amm);
   }
   return 0;
}

static long Layoutbutton(struct Button *but,struct Amlayout *aml)
{  if(but->body)
   {  if(but->flags&BUTF_COMPLETE)
      {  long winw,bodyw;
         if(!(aml->flags&(AMLF_BREAK|AMLF_RETRY)))
         {  /* Make button its max width unless we are first in the line, then make it fit. */
            if(aml->flags&AMLF_FIRST)
            {  winw=aml->width-aml->startx-2*bevelw;
               if(but->maxw<winw) bodyw=but->maxw;
               else if(but->minw>winw) bodyw=but->minw;
               else bodyw=winw;
            }
            else
            {  bodyw=but->maxw;
            }
            Alayout(but->body,bodyw,aml->height,0,aml->text,0,NULL);
            but->aow=Agetattr(but->body,AOBJ_Width)+2*bevelw;
            but->aoh=Agetattr(but->body,AOBJ_Height)+2*bevelh;
         }
         AmethodasA(AOTP_FIELD,but,aml);
      }
   }
   else
   {  AmethodasA(AOTP_FIELD,but,aml);
   }
   return 0;
}

static long Alignbutton(struct Button *but,struct Amalign *ama)
{  if(but->body)
   {  if(but->flags&BUTF_COMPLETE)
      {  AmethodasA(AOTP_FIELD,but,ama);
         Amove(but->body,but->aox+bevelw,but->aoy+bevelh);
      }
   }
   else
   {  AmethodasA(AOTP_FIELD,but,ama);
   }
   return 0;
}

static long Renderbutton(struct Button *but,struct Amrender *amr)
{  struct Coords *coo,coords={0};
   BOOL clip=FALSE;
   ULONG clipkey=NULL;
   struct RastPort *rp;
   ULONG state=IDS_NORMAL;
   struct ColorMap *colormap=NULL;
   struct TextFont *font;
   long bpen=~0,tpen=~0;
   long textw,textl,textx,buttonw;
   struct TextExtent te={0};
   if(!(coo=amr->coords))
   {  Framecoords(but->cframe,&coords);
      coo=&coords;
      clip=TRUE;
   }
   if(coo->rp)
   {  rp=coo->rp;
      Agetattrs(Aweb(),
         AOAPP_Colormap,&colormap,
         AOAPP_Screenfont,&font,
         TAG_END);
      if(but->flags&BUTF_SELECTED)
      {  state=IDS_SELECTED;
         tpen=coo->dri->dri_Pens[FILLTEXTPEN];
         bpen=coo->dri->dri_Pens[FILLPEN];
      }
      if(clip) clipkey=Clipto(rp,coo->minx,coo->miny,coo->maxx,coo->maxy);
      SetAttrs(bevel,
         IA_Width,but->aow,
         IA_Height,but->aoh,
         BEVEL_FillPen,bpen,
         BEVEL_TextPen,tpen,
         BEVEL_ColorMap,colormap,
         BEVEL_Flags,BFLG_XENFILL,
         CLASSACT_SpecialPens,but->capens,
         TAG_END);
      DrawImageState(rp,bevel,but->aox+coo->dx,but->aoy+coo->dy,state,coo->dri);
      if(but->body)
      {  if(but->flags&BUTF_COMPLETE)
         {  if(bpen==~0) bpen=coo->dri->dri_Pens[BACKGROUNDPEN];
            Asetattrs(but->body,AOBDY_Forcebgcolor,bpen,TAG_END);
            /* Always render the entire button contents since the bevel has cleared
             * the background */
            Arender(but->body,coo,0,0,AMRMAX,AMRMAX,0,amr->text);
         }
      }
      else
      {  SetFont(rp,font);
         SetSoftStyle(rp,0,0x0f);
         SetAPen(rp,coo->dri->dri_Pens[TEXTPEN]);
         /* With JS the value can have changed after measure */
         textl=strlen(but->value);
         textw=Textlength(rp,but->value,textl);
         buttonw=but->aow-2*bevelw-8;
         if(textw<=buttonw)
         {  /* Text still fits, center it */
            textx=(buttonw-textw)/2;
         }
         else
         {  /* Text doesn't fit, find out how much fits */
            textl=TextFit(rp,but->value,textl,&te,NULL,1,buttonw,but->aoh);
            textx=0;
         }
         Move(rp,but->aox+coo->dx+bevelw+4+textx,but->aoy+coo->dy+bevelh+1+rp->TxBaseline);
         Text(rp,but->value,textl);
      }
      if(clip) Unclipto(clipkey);
   }
   return 0;
}

static long Setbutton(struct Button *but,struct Amset *ams)
{  long result;
   struct TagItem *tag,*tstate=ams->tags;
   BOOL newcframe=FALSE,newframe=FALSE,newwin=FALSE,custom=FALSE;
   while(tag=NextTagItem(&tstate))
   {  switch(tag->ti_Tag)
      {  case AOBUT_Type:
            but->type=tag->ti_Data;
            break;
         case AOBJ_Window:
            if(tag->ti_Data)
            {  but->capens=(void *)Agetattr((void *)tag->ti_Data,AOWIN_Specialpens);
            }
            else
            {  but->capens=NULL;
               but->flags&=~BUTF_SELECTED;
            }
            newwin=TRUE;
            break;
         case AOBJ_Frame:
            if(!tag->ti_Data)
            {  but->flags|=BUTF_REMEASURE;
            }
            but->frame=(void *)tag->ti_Data;
            newframe=TRUE;
            break;
         case AOBJ_Cframe:
            newcframe=TRUE;
            break;
         case AOFLD_Onclick:
            if(but->onclick) FREE(but->onclick);
            but->onclick=Dupstr((UBYTE *)tag->ti_Data,-1);
            break;
         case AOFLD_Onfocus:
            if(but->onfocus) FREE(but->onfocus);
            but->onfocus=Dupstr((UBYTE *)tag->ti_Data,-1);
            break;
         case AOFLD_Onblur:
            if(but->onblur) FREE(but->onblur);
            but->onblur=Dupstr((UBYTE *)tag->ti_Data,-1);
            break;
         case AOBJ_Pool:
            but->pool=(void *)tag->ti_Data;
            break;
         case AOBUT_Custom:
            if(tag->ti_Data && !but->body) custom=TRUE;
            break;
         case AOBUT_Complete:
            SETFLAG(but->flags,BUTF_COMPLETE,tag->ti_Data);
            break;
         case AOBJ_Layoutparent:
            but->parent=(void *)tag->ti_Data;
            break;
         case AOBJ_Changedchild:
            if((void *)tag->ti_Data==but->body)
            {  Asetattrs(but->parent,AOBJ_Changedchild,but,TAG_END);
            }
            break;
      }
   }
   /* Superclass does AOM_ADDCHILD which needs type */
   result=Amethodas(AOTP_FIELD,but,AOM_SET,ams->tags);
   if(!but->value)
   {  switch(but->type)
      {  case BUTTP_SUBMIT:
            but->value=Dupstr(AWEBSTR(MSG_AWEB_FORMSUBMIT),-1);
            break;
         case BUTTP_RESET:
            but->value=Dupstr(AWEBSTR(MSG_AWEB_FORMRESET),-1);
            break;
         default:
            but->value=Dupstr(AWEBSTR(MSG_AWEB_FORMBUTTON),-1);
      }
   }
   if(custom && !but->body)
   {  but->body=Anewobject(AOTP_BODY,
         AOBJ_Pool,but->pool,
         AOBJ_Frame,but->frame,
         AOBJ_Cframe,but->cframe,
         AOBJ_Window,but->win,
         AOBJ_Layoutparent,but,
         AOBDY_Leftmargin,2,
         AOBDY_Topmargin,1,
         TAG_END);
   }
   if(but->body && (newcframe || newframe || newwin))
   {  Asetattrs(but->body,
         newcframe?AOBJ_Cframe:TAG_IGNORE,but->cframe,
         newframe?AOBJ_Frame:TAG_IGNORE,but->frame,
         newwin?AOBJ_Window:TAG_IGNORE,but->win,
         TAG_END);
   }
   return result;
}

static long Getbutton(struct Button *but,struct Amset *ams)
{  long result;
   struct TagItem *tag,*tstate=ams->tags;
   result=AmethodasA(AOTP_FIELD,but,ams);
   while(tag=NextTagItem(&tstate))
   {  switch(tag->ti_Tag)
      {  case AOBUT_Type:
            PUTATTR(tag,but->type);
            break;
         case AOBUT_Body:
            PUTATTR(tag,but->body);
            break;
      }
   }
   return result;
}

static struct Button *Newbutton(struct Amset *ams)
{  struct Button *but;
   if(but=Allocobject(AOTP_BUTTON,sizeof(struct Button),ams))
   {  Setbutton(but,ams);
   }
   return but;
}

static long Hittestbutton(struct Button *but,struct Amhittest *amh)
{  long result=AMHR_NOHIT;
   long popup=0;
   if(but->body && (but->flags&BUTF_COMPLETE))
   {  result=AmethodA(but->body,amh);
      popup=result&AMHR_POPUP;
      result&=~AMHR_POPUP;
   }
   if(!result)
   {  if(amh->oldobject==but)
      {  result=AMHR_OLDHIT;
      }
      else
      {  result=AMHR_NEWHIT;
         if(amh->amhr)
         {  amh->amhr->object=but;
            if(but->type==BUTTP_SUBMIT)
            {  void *url=(void *)Agetattr(but->form,AOFOR_Action);
               if(url)
               {  amh->amhr->text=Dupstr((UBYTE *)Agetattr(url,AOURL_Url),-1);
               }
            }
         }
      }
   }
   return result|popup;
}

static long Goactivebutton(struct Button *but,struct Amgoactive *amg)
{  but->flags|=BUTF_SELECTED;
   Arender(but,NULL,0,0,AMRMAX,AMRMAX,0,NULL);
   return AMR_ACTIVE;
}

static long Handleinputbutton(struct Button *but,struct Aminput *ami)
{  struct Coords coords={0};
   long result=AMR_REUSE;
   long x,y;
   if(ami->imsg)
   {  switch(ami->imsg->Class)
      {  case IDCMP_MOUSEMOVE:
         case IDCMP_RAWKEY:
            /* Check if mouse is still over us */
            Framecoords(but->cframe,&coords);
            x=ami->imsg->MouseX-coords.dx;
            y=ami->imsg->MouseY-coords.dy;
            if(x>=but->aox && x<but->aox+but->aow
            && y>=but->aoy && y<but->aoy+but->aoh)
            {  if(!(but->flags&BUTF_SELECTED))
               {  but->flags|=BUTF_SELECTED;
                  Arender(but,NULL,0,0,AMRMAX,AMRMAX,0,NULL);
               }
               result=AMR_ACTIVE;
            }
            else
            {  if(but->flags&BUTF_SELECTED)
               {  but->flags&=~BUTF_SELECTED;
                  Arender(but,NULL,0,0,AMRMAX,AMRMAX,0,NULL);
               }
               result=AMR_ACTIVE;
            }
            break;
         case IDCMP_MOUSEBUTTONS:
            if(ami->imsg->Code==SELECTUP && (but->flags&BUTF_SELECTED))
            {  Clickbutton(but,but->onclick || AWebJSBase);
            }
            result=AMR_NOREUSE;
            break;
         case IDCMP_INTUITICKS:
            result=AMR_ACTIVE;
            break;
      }
   }
   return result;
}

static long Goinactivebutton(struct Button *but)
{  if(but->flags&BUTF_SELECTED)
   {  but->flags&=~BUTF_SELECTED;
      Arender(but,NULL,0,0,AMRMAX,AMRMAX,0,NULL);
   }
   return 0;
}

static long Notifybutton(struct Button *but,struct Amnotify *amn)
{  if(but->body)
   {  AmethodA(but->body,amn);
   }
   return (long)AmethodasA(AOTP_FIELD,but,amn);
}

static long Jsetupbutton(struct Button *but,struct Amjsetup *amj)
{  struct Jvar *jv;
   UBYTE *p;
   AmethodasA(AOTP_FIELD,but,amj);
   if(but->jobject)
   {  if(jv=Jproperty(amj->jc,but->jobject,"value"))
      {  Setjproperty(jv,Propertyvalue,but);
      }
      if(jv=Jproperty(amj->jc,but->jobject,"type"))
      {  Setjproperty(jv,JPROPHOOK_READONLY,NULL);
         switch(but->type)
         {  case BUTTP_SUBMIT:p="submit";break;
            case BUTTP_RESET: p="reset";break;
            case BUTTP_BUTTON:p="button";break;
            default:          p="unknown";
         }
         Jasgstring(amj->jc,jv,p);
      }
      Addjfunction(amj->jc,but->jobject,"click",Methodclick,NULL);
      Addjfunction(amj->jc,but->jobject,"focus",Methodfocus,NULL);
      Addjfunction(amj->jc,but->jobject,"blur",Methodblur,NULL);
      Addjfunction(amj->jc,but->jobject,"toString",Methodtostring,NULL);
      Jaddeventhandler(amj->jc,but->jobject,"onclick",but->onclick);
   }
   if(but->body)
   {  /* Don't add the label contents to the form object, but wait until
       * we are called again with the document object as parent. */
      struct Jobject *fjo=(struct Jobject *)Agetattr(but->form,AOBJ_Jobject);
      if(amj->parent!=fjo)
      {  AmethodA(but->body,amj);
      }
   }
   return 0;
}

static void Disposebutton(struct Button *but)
{  if(but->onclick) FREE(but->onclick);
   if(but->onfocus) FREE(but->onfocus);
   if(but->onblur) FREE(but->onblur);
   if(but->body) Adisposeobject(but->body);
   Amethodas(AOTP_FIELD,but,AOM_DISPOSE);
}

static void Deinstallbutton(void)
{  if(bevel) DisposeObject(bevel);
}

static long Dispatch(struct Button *but,struct Amessage *amsg)
{  long result=0;
   switch(amsg->method)
   {  case AOM_NEW:
         result=(long)Newbutton((struct Amset *)amsg);
         break;
      case AOM_SET:
         result=Setbutton(but,(struct Amset *)amsg);
         break;
      case AOM_GET:
         result=Getbutton(but,(struct Amset *)amsg);
         break;
      case AOM_MEASURE:
         result=Measurebutton(but,(struct Ammeasure *)amsg);
         break;
      case AOM_LAYOUT:
         result=Layoutbutton(but,(struct Amlayout *)amsg);
         break;
      case AOM_ALIGN:
         result=Alignbutton(but,(struct Amalign *)amsg);
         break;
      case AOM_RENDER:
         result=Renderbutton(but,(struct Amrender *)amsg);
         break;
      case AOM_HITTEST:
         result=Hittestbutton(but,(struct Amhittest *)amsg);
         break;
      case AOM_GOACTIVE:
         result=Goactivebutton(but,(struct Amgoactive *)amsg);
         break;
      case AOM_HANDLEINPUT:
         result=Handleinputbutton(but,(struct Aminput *)amsg);
         break;
      case AOM_GOINACTIVE:
         result=Goinactivebutton(but);
         break;
      case AOM_NOTIFY:
         result=Notifybutton(but,(struct Amnotify *)amsg);
         break;
      case AOM_JSETUP:
         result=Jsetupbutton(but,(struct Amjsetup *)amsg);
         break;
      case AOM_DISPOSE:
         Disposebutton(but);
         break;
      case AOM_DEINSTALL:
         Deinstallbutton();
         break;
      default:
         result=AmethodasA(AOTP_FIELD,but,amsg);
   }
   return result;
}

/*------------------------------------------------------------------------*/

BOOL Installbutton(void)
{  if(!Amethod(NULL,AOM_INSTALL,AOTP_BUTTON,Dispatch)) return FALSE;
   if(!(bevel=BevelObject,
      BEVEL_Style,BVS_BUTTON,
      End)) return FALSE;
   GetAttr(BEVEL_VertSize,bevel,(ULONG *)&bevelw);
   GetAttr(BEVEL_HorizSize,bevel,(ULONG *)&bevelh);
   return TRUE;
}
