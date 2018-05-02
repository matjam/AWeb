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

/* imgcopy.c - AWeb image copy driver object */

#include "aweb.h"
#include "imgprivate.h"
#include "copydriver.h"
#include "copy.h"
#include <graphics/scale.h>
#include <clib/graphics_protos.h>
#include <clib/utility_protos.h>

/*------------------------------------------------------------------------*/

struct Imgcopy
{  struct Copydriver cdv;
   void *copy;
   long swidth,sheight;          /* Suggested width,height or 0 */
   struct Imgsource *source;
   struct BitMap *bitmap;        /* Same as source's bitmap, or our own (scaled) */
   UBYTE *mask;                  /* Same as source's mask */
   long width,height;            /* Size of bitmap */
   USHORT flags;
};

#define IMGF_OURBITMAP  0x0001   /* Bitmap is ours */

/*------------------------------------------------------------------------*/
/*------------------------------------------------------------------------*/

/* Source has a new bitmap for us (or none at all) */
static void Newbitmap(struct Imgcopy *img)
{  if(img->flags&IMGF_OURBITMAP)
   {  if(img->bitmap) FreeBitMap(img->bitmap);
   }
   img->flags&=~IMGF_OURBITMAP;

   if(img->source->bitmap)
   {  if(img->swidth && !img->sheight)
      {  img->sheight=img->source->height*img->swidth/img->source->width;
         if(img->sheight<1) img->sheight=1;
      }
      if(img->sheight && !img->swidth)
      {  img->swidth=img->source->width*img->sheight/img->source->height;
         if(img->swidth<1) img->swidth=1;
      }
   }

   if(img->source->bitmap && img->swidth && img->sheight && !img->source->mask
   && (img->swidth!=img->source->width || img->sheight!=img->source->height))
   {  /* Create our own scaled bitmap */
      if(img->bitmap=AllocBitMap(img->swidth,img->sheight,img->source->depth,
         BMF_MINPLANES,img->source->bitmap))
      {  struct BitScaleArgs bsa={0};
         bsa.bsa_SrcX=0;
         bsa.bsa_SrcY=0;
         bsa.bsa_SrcWidth=img->source->width;
         bsa.bsa_SrcHeight=img->source->height;
         bsa.bsa_DestX=0;
         bsa.bsa_DestY=0;
         bsa.bsa_XSrcFactor=img->source->width;
         bsa.bsa_XDestFactor=img->swidth;
         bsa.bsa_YSrcFactor=img->source->height;
         bsa.bsa_YDestFactor=img->sheight;
         bsa.bsa_SrcBitMap=img->source->bitmap;
         bsa.bsa_DestBitMap=img->bitmap;
         bsa.bsa_Flags=0;
         BitMapScale(&bsa);
         img->flags|=IMGF_OURBITMAP;
         img->mask=NULL;
         img->width=img->swidth;
         img->height=img->sheight;
      }
   }

   if(!(img->flags&IMGF_OURBITMAP))
   {  img->bitmap=img->source->bitmap;
      img->mask=img->source->mask;
      img->width=img->source->width;
      img->height=img->source->height;
   }
}

/*------------------------------------------------------------------------*/

static long Measureimgcopy(struct Imgcopy *img,struct Ammeasure *amm)
{  if(img->bitmap)
   {  img->aow=img->width;
      img->aoh=img->height;
      if(amm->ammr)
      {  amm->ammr->width=amm->ammr->minwidth=img->aow;
      }
   }
   return 0;
}

static long Layoutimgcopy(struct Imgcopy *img,struct Amlayout *aml)
{  if(img->bitmap)
   {  img->aow=img->width;
      img->aoh=img->height;
   }
   return 0;
}

static long Renderimgcopy(struct Imgcopy *img,struct Amrender *amr)
{  struct Coords *coo=NULL;
	long dx,dy,w,h;
   if(img->bitmap && !(amr->flags&(AMRF_UPDATESELECTED|AMRF_UPDATENORMAL)))
   {  coo=Clipcoords(img->cframe,amr->coords);
      if(coo && coo->rp)
      {  if(amr->flags&AMRF_CLEAR)
         {  Erasebg(img->cframe,coo,amr->minx,amr->miny,amr->maxx,amr->maxy);
         }
         if(img->aox<=amr->maxx && img->aoy<=amr->maxy
         && img->aox+img->width>amr->minx && img->aoy+img->height>amr->miny)
         {  dx=MAX(0,amr->minx-img->aox);
            dy=MAX(0,amr->miny-img->aoy);
            w=MIN(amr->maxx-img->aox+1,img->width)-dx;
            h=MIN(amr->maxy-img->aoy+1,img->height)-dy;
            if(img->mask)
	         {  BltMaskBitMapRastPort(img->bitmap,dx,dy,coo->rp,
                  img->aox+dx+coo->dx,img->aoy+dy+coo->dy,w,h,
                  0xe0,img->mask);
            }
            else
            {  BltBitMapRastPort(img->bitmap,dx,dy,coo->rp,
                  img->aox+dx+coo->dx,img->aoy+dy+coo->dy,w,h,
                  0xc0);
            }
         }
      }
      Unclipcoords(coo);
   }
   /* If we render, we are always completely decoded. Also, no animations with datatypes. */
   Asetattrs(img->copy,AOCPY_Onimgload,TRUE,TAG_END);
   return 0;
}

static long Setimgcopy(struct Imgcopy *img,struct Amset *ams)
{  long result=0;
   struct TagItem *tag,*tstate=ams->tags;
   BOOL rescale=FALSE;
   Amethodas(AOTP_COPYDRIVER,img,AOM_SET,ams->tags);
   while(tag=NextTagItem(&tstate))
   {  switch(tag->ti_Tag)
      {  case AOCDV_Copy:
            img->copy=(void *)tag->ti_Data;
            break;
         case AOCDV_Sourcedriver:
            img->source=(struct Imgsource *)tag->ti_Data;
            break;
         case AOCDV_Width:
            if(tag->ti_Data && img->width!=tag->ti_Data) rescale=TRUE;
            img->swidth=tag->ti_Data;
            break;
         case AOCDV_Height:
            if(tag->ti_Data && img->height!=tag->ti_Data) rescale=TRUE;
            img->sheight=tag->ti_Data;
            break;
         case AOIMP_Srcupdate:
            Newbitmap(img);
            Asetattrs(img->copy,
               AOBJ_Changedchild,img,
               AOCPY_Onimgload,TRUE,
               TAG_END);
            break;

/*
#ifdef DEVELOPER
case AOCDV_Objectparams:
{ struct Objectparam *op=(struct Objectparam *)tag->ti_Data;
for(;op->next;op=op->next)
{ printf("Param name=%s value=%s vtype=%s type=%s\n",
   op->name?op->name:NULLSTRING,op->value?op->value:NULLSTRING,
   op->valuetype?op->valuetype:NULLSTRING,op->type?op->type:NULLSTRING);
}
}
break;
#endif
*/
      }
   }
   if(rescale) Newbitmap(img);
   return result;
}

static void Disposeimgcopy(struct Imgcopy *img)
{  if(img->flags&IMGF_OURBITMAP)
   {  if(img->bitmap) FreeBitMap(img->bitmap);
   }
   Amethodas(AOTP_COPYDRIVER,img,AOM_DISPOSE);
}

static struct Imgcopy *Newimgcopy(struct Amset *ams)
{  struct Imgcopy *img=Allocobject(AOTP_IMGCOPY,sizeof(struct Imgcopy),ams);
   if(img)
   {  Setimgcopy(img,ams);
   }
   return img;
}

static long Getimgcopy(struct Imgcopy *img,struct Amset *ams)
{  struct TagItem *tag,*tstate=ams->tags;
   long result;
   result=AmethodasA(AOTP_COPYDRIVER,img,ams);
   while(tag=NextTagItem(&tstate))
   {  switch(tag->ti_Tag)
      {  case AOCDV_Ready:
            PUTATTR(tag,BOOLVAL(img->bitmap));
            break;
         case AOCDV_Imagebitmap:
            PUTATTR(tag,img->bitmap);
            break;
         case AOCDV_Imagemask:
            PUTATTR(tag,img->mask);
            break;
         case AOCDV_Imagewidth:
            PUTATTR(tag,img->bitmap?img->width:0);
            break;
         case AOCDV_Imageheight:
            PUTATTR(tag,img->bitmap?img->height:0);
            break;
      }
   }
   return result;
}

static long Dispatch(struct Imgcopy *img,struct Amessage *amsg)
{  long result=0;
   switch(amsg->method)
   {  case AOM_NEW:
         result=(long)Newimgcopy((struct Amset *)amsg);
         break;
      case AOM_SET:
         result=Setimgcopy(img,(struct Amset *)amsg);
         break;
      case AOM_GET:
         result=Getimgcopy(img,(struct Amset *)amsg);
         break;
      case AOM_MEASURE:
         result=Measureimgcopy(img,(struct Ammeasure *)amsg);
         break;
      case AOM_LAYOUT:
         result=Layoutimgcopy(img,(struct Amlayout *)amsg);
         break;
      case AOM_RENDER:
         result=Renderimgcopy(img,(struct Amrender *)amsg);
         break;
      case AOM_DISPOSE:
         Disposeimgcopy(img);
         break;
      case AOM_DEINSTALL:
         break;
      default:
         result=AmethodasA(AOTP_COPYDRIVER,img,amsg);
   }
   return result;
}

/*------------------------------------------------------------------------*/

BOOL Installimgcopy(void)
{  if(!Amethod(NULL,AOM_INSTALL,AOTP_IMGCOPY,Dispatch)) return FALSE;
   return TRUE;
}

