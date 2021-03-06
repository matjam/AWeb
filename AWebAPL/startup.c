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

/* startup.c - AWeb startup window AWebLib module */

#include "aweblib.h"
#include "application.h"
#include "startup.h"
#include "classact.h"
#include <exec/resident.h>
#include <clib/exec_protos.h>
#include <clib/alib_protos.h>
#include <clib/intuition_protos.h>

static void *AwebPluginBase;
void *IntuitionBase,*UtilityBase,*DOSBase;
struct ClassLibrary *WindowBase,*LayoutBase,*ButtonBase,
   *LabelBase,*PenMapBase,*FuelGaugeBase;

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

__asm __saveds void doStartupopen(
   register __a0 struct Screen *screen,
   register __a1 UBYTE *version);

__asm __saveds void doStartupstate(
   register __d0 ULONG state);

__asm __saveds void doStartuplevel(
   register __d0 long ready,
   register __d1 long total);

__asm __saveds void doStartupclose(void);

/* Function declarations for project dependent hook functions */
static ULONG Initaweblib(struct Library *libbase);
static void Expungeaweblib(struct Library *libbase);

struct Library *StartupBase;

static APTR libseglist;

struct ExecBase *SysBase;

LONG __saveds __asm Libstart(void)
{  return -1;
}

static APTR functable[]=
{  Openlib,
   Closelib,
   Expungelib,
   Extfunclib,
   doStartupopen,
   doStartupstate,
   doStartuplevel,
   doStartupclose,
   (APTR)-1
};

/* Init table used in library initialization. */
static ULONG inittab[]=
{  sizeof(struct Library),
   (ULONG) functable,
   0,
   (ULONG) Initlib
};

static char __aligned libname[]="startup.aweblib";
static char __aligned libid[]="startup.aweblib " AWEBLIBVSTRING " " __AMIGADATE__;

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
   StartupBase=libbase;
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
   if(libbase->lib_OpenCnt==1)
   {  AwebPluginBase=OpenLibrary("awebplugin.library",0);
   }
#ifndef DEMOVERSION
   if(!Fullversion())
   {  Closelib(libbase);
      return NULL;
   }
#endif
   return libbase;
}

__asm __saveds struct SegList *Closelib(
   register __a6 struct Library *libbase)
{  libbase->lib_OpenCnt--;
   if(libbase->lib_OpenCnt==0)
   {  if(AwebPluginBase)
      {  CloseLibrary(AwebPluginBase);
         AwebPluginBase=NULL;
      }
      if(libbase->lib_Flags&LIBF_DELEXP)
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
{  if(!(IntuitionBase=OpenLibrary("intuition.library",39))) return FALSE;
   if(!(UtilityBase=OpenLibrary("utility.library",39))) return FALSE;
   if(!(WindowBase=(struct ClassLibrary *)OpenLibrary("window.class",OSNEED(0,44)))) return FALSE;
   if(!(LayoutBase=(struct ClassLibrary *)OpenLibrary("gadgets/layout.gadget",OSNEED(0,44)))) return FALSE;
   if(!(ButtonBase=(struct ClassLibrary *)OpenLibrary("gadgets/button.gadget",OSNEED(0,44)))) return FALSE;
   if(!(FuelGaugeBase=(struct ClassLibrary *)OpenLibrary("gadgets/fuelgauge.gadget",OSNEED(0,44)))) return FALSE;
   if(!(PenMapBase=(struct ClassLibrary *)OpenLibrary("images/penmap.image",OSNEED(0,44)))) return FALSE;
   if(!(LabelBase=(struct ClassLibrary *)OpenLibrary("images/label.image",OSNEED(0,44)))) return FALSE;
   return TRUE;
}

static void Expungeaweblib(struct Library *libbase)
{  if(LabelBase) CloseLibrary(LabelBase);
   if(PenMapBase) CloseLibrary(PenMapBase);
   if(FuelGaugeBase) CloseLibrary(FuelGaugeBase);
   if(ButtonBase) CloseLibrary(ButtonBase);
   if(LayoutBase) CloseLibrary(LayoutBase);
   if(WindowBase) CloseLibrary(WindowBase);
   if(UtilityBase) CloseLibrary(UtilityBase);
   if(IntuitionBase) CloseLibrary(IntuitionBase);
}

/*-----------------------------------------------------------------------*/

/* Logo palette for penmap */
ULONG aweblogopalette[] =
{
   4,
   0x6060606, 0x6060606, 0x6060606,
   0x12121212, 0x35353535, 0xC7C7C7C7,
   0xCDCDCDCD, 0x1F1F1F1F, 0x20202020,
   0xAAAAAAAA, 0xAAAAAAAA, 0xAAAAAAAA,
   0x0, 0x0, 0x0
};

/* Logo in penmap chunky packed form.
 * First 4 bytes are exact.
 * Each following byte contains 6 bits length (add 1) and 2 bits data. */
static UBYTE logopacked[]=
{  0,192,0,93,
   0xff,0xff,0xff,0xff,0xff,0xff,0xff,0xff,0xff,
   0xff,0xff,0xff,0xff,0xff,0xff,0xef,0x06,0xff,
   0xff,0xf3,0x0a,0xff,0xff,0xeb,0x16,0xff,0xff,
   0xdf,0x22,0xff,0xff,0xd3,0x2a,0xff,0xff,0xcf,
   0x32,0xff,0xff,0xc3,0x3a,0xff,0xff,0xbb,0x46,
   0xff,0xff,0xaf,0x4e,0xff,0xff,0xab,0x56,0xff,
   0xff,0x9f,0x62,0xff,0xff,0x73,0x08,0x1b,0x62,
   0x17,0x38,0x1b,0x1c,0x23,0x08,0x1b,0x34,0x6f,
   0x24,0xc3,0x0c,0x1f,0x62,0x1f,0x1c,0x2b,0x1c,
   0x1f,0x08,0x33,0x08,0x87,0x24,0xc3,0x0c,0x23,
   0x5e,0x1f,0x1c,0x2b,0x1c,0x1b,0x08,0x37,0x04,
   0x97,0x14,0xc7,0x10,0x1f,0x62,0x1b,0x1c,0x2b,
   0x1c,0x17,0x08,0x37,0x08,0x93,0x18,0xc3,0x14,
   0x23,0x5e,0x1b,0x1c,0x2b,0x1c,0x13,0x0c,0x33,
   0x08,0x97,0x14,0xc7,0x14,0x23,0x62,0x17,0x1c,
   0x2b,0x1c,0x13,0x08,0x33,0x08,0x9b,0x14,0xc3,
   0x18,0x27,0x62,0x13,0x1c,0x2b,0x1c,0x0f,0x08,
   0x37,0x08,0x97,0x18,0xc3,0x1c,0x23,0x62,0x13,
   0x1c,0x2b,0x1c,0x0b,0x08,0x37,0x08,0x9b,0x14,
   0xc7,0x1c,0x27,0x56,0x1b,0x1c,0x2b,0x1c,0x07,
   0x08,0x37,0x08,0x9f,0x14,0xc3,0x20,0x2b,0x4e,
   0x1f,0x1c,0x2b,0x1c,0x03,0x08,0x37,0x08,0x9f,
   0x14,0xc7,0x20,0x2b,0x46,0x27,0x1c,0x2b,0x1c,
   0x03,0x08,0x37,0x04,0xa3,0x14,0xc7,0x24,0x2b,
   0x3a,0x2f,0x1c,0x2b,0x28,0x37,0x08,0x9f,0x18,
   0xc3,0x28,0x2b,0x32,0x37,0x1c,0x2b,0x24,0x37,
   0x08,0xa3,0x14,0xc7,0x28,0x2f,0x2a,0x3b,0x1c,
   0x2b,0x20,0x37,0x08,0xa3,0x18,0xc3,0x30,0x2b,
   0x22,0x43,0x1c,0x2b,0x1c,0x3b,0x04,0xa7,0x14,
   0xc7,0x30,0x2f,0x16,0x4b,0x1c,0x2b,0x1c,0x37,
   0x08,0xa7,0x14,0xc7,0x30,0x33,0x0a,0x53,0x1c,
   0x2b,0x1c,0x33,0x08,0xa7,0x18,0xc3,0x08,0x03,
   0x28,0x2f,0x06,0x57,0x1c,0x2b,0x1c,0x2f,0x08,
   0x47,0x1c,0x43,0x14,0x17,0x14,0x97,0x08,0x03,
   0x28,0x8f,0x1c,0x2b,0x1c,0x2f,0x04,0x3f,0x30,
   0x3b,0x14,0x0b,0x2c,0x87,0x08,0x0b,0x24,0x8f,
   0x1c,0x2b,0x1c,0x2b,0x08,0x33,0x14,0x17,0x14,
   0x2f,0x14,0x07,0x0c,0x17,0x10,0x83,0x08,0x0b,
   0x24,0x8f,0x1c,0x27,0x20,0x27,0x08,0x33,0x10,
   0x23,0x14,0x2b,0x14,0x03,0x08,0x23,0x10,0x7f,
   0x08,0x0b,0x28,0x8b,0x1c,0x27,0x20,0x23,0x08,
   0x2f,0x14,0x27,0x14,0x27,0x20,0x2b,0x14,0x77,
   0x08,0x13,0x24,0x8b,0x1c,0x23,0x24,0x23,0x04,
   0x2f,0x14,0x2b,0x18,0x23,0x1c,0x33,0x10,0x77,
   0x08,0x13,0x24,0x8b,0x1c,0x1f,0x28,0x1f,0x08,
   0x2b,0x10,0x33,0x18,0x23,0x18,0x33,0x18,0x73,
   0x04,0x17,0x24,0x8b,0x1c,0x1b,0x08,0x03,0x1c,
   0x1b,0x08,0x27,0x18,0x33,0x18,0x1f,0x18,0x37,
   0x18,0x6f,0x08,0x17,0x28,0x87,0x1c,0x1b,0x04,
   0x07,0x1c,0x17,0x08,0x2b,0x14,0x37,0x18,0x1f,
   0x18,0x37,0x18,0x6f,0x08,0x1b,0x24,0x87,0x1c,
   0x17,0x08,0x07,0x1c,0x13,0x08,0x2b,0x18,0x37,
   0x18,0x1f,0x14,0x3b,0x18,0x6b,0x08,0x1f,0x24,
   0x87,0x1c,0x13,0x08,0x0b,0x1c,0x13,0x08,0x27,
   0x6c,0x1f,0x14,0x3f,0x18,0x6b,0x08,0x1f,0x28,
   0x83,0x1c,0x0f,0x08,0x0f,0x1c,0x0f,0x08,0x2b,
   0x6c,0x1f,0x14,0x3b,0x1c,0x6b,0x08,0x1f,0x28,
   0x83,0x1c,0x0f,0x04,0x13,0x1c,0x0b,0x08,0x2b,
   0x18,0x73,0x18,0x3b,0x18,0x6b,0x08,0x27,0x24,
   0x83,0x1c,0x0b,0x08,0x13,0x1c,0x0b,0x04,0x2b,
   0x1c,0x73,0x14,0x3f,0x18,0x6b,0x08,0x27,0x24,
   0x83,0x1c,0x07,0x08,0x17,0x1c,0x07,0x04,0x2f,
   0x1c,0x73,0x14,0x3b,0x1c,0x67,0x08,0x2b,0x28,
   0x7f,0x1c,0x03,0x08,0x1b,0x1c,0x03,0x08,0x2f,
   0x18,0x73,0x14,0x3f,0x18,0x6b,0x08,0x2f,0x24,
   0x7f,0x28,0x1f,0x28,0x2f,0x1c,0x73,0x14,0x3b,
   0x18,0x6b,0x08,0x33,0x24,0x7f,0x24,0x23,0x24,
   0x33,0x1c,0x6f,0x14,0x3f,0x18,0x6b,0x08,0x33,
   0x28,0x7b,0x20,0x27,0x20,0x37,0x18,0x4b,0x04,
   0x1f,0x14,0x3b,0x18,0x6f,0x08,0x37,0x24,0x7b,
   0x1c,0x2b,0x1c,0x3b,0x18,0x47,0x08,0x1f,0x14,
   0x3b,0x14,0x6f,0x08,0x3b,0x24,0x7b,0x1c,0x2b,
   0x18,0x43,0x14,0x43,0x08,0x1f,0x18,0x37,0x14,
   0x73,0x08,0x3b,0x28,0x77,0x18,0x2f,0x18,0x43,
   0x14,0x3f,0x08,0x23,0x18,0x33,0x14,0x77,0x08,
   0x3b,0x28,0x77,0x14,0x33,0x14,0x4b,0x10,0x3b,
   0x08,0x23,0x20,0x2b,0x14,0x77,0x08,0x43,0x24,
   0x77,0x10,0x37,0x10,0x4f,0x14,0x2f,0x0c,0x27,
   0x20,0x27,0x14,0x7b,0x08,0x43,0x24,0x77,0x0c,
   0x3b,0x0c,0x57,0x14,0x1f,0x10,0x2f,0x10,0x07,
   0x08,0x1b,0x14,0x7f,0x0c,0x43,0x28,0x73,0x0c,
   0x3b,0x0c,0x5f,0x38,0x33,0x14,0x0b,0x30,0x87,
   0x7c,0xff,0x3b,0x20,0x3f,0x14,0x17,0x14,0x93,
   0x80,0xff,0xff,0x7b,0x84,0xff,0xff,0x77,0x08,
   0x4f,0x28,0xff,0xff,0x73,0x0c,0x53,0x24,0xff,
   0xff,0x73,0x08,0x57,0x24,0xff,0xff,0x73,0x08,
   0x57,0x28,0xff,0xff,0x6b,0x0c,0x57,0x28,0xff,
   0xff,0x6b,0x08,0x5f,0x24,0xff,0x77,0x01,0xef,
   0x08,0x5f,0x24,0x77,0x0d,0x4b,0x01,0x3f,0x0d,
   0x4f,0x01,0x3b,0x11,0x4b,0x01,0x4b,0x08,0x63,
   0x28,0x6b,0x1d,0x43,0x01,0x37,0x1d,0x43,0x05,
   0x37,0x19,0x47,0x01,0x4b,0x08,0x63,0x28,0x67,
   0x25,0x3b,0x05,0x33,0x25,0x3f,0x05,0x33,0x25,
   0x3b,0x05,0x47,0x0c,0x67,0x24,0x63,0x2d,0x33,
   0x09,0x2f,0x2d,0x37,0x09,0x2f,0x2d,0x33,0x09,
   0x47,0x08,0x6b,0x28,0x5f,0x31,0x2f,0x09,0x2f,
   0x31,0x2f,0x0d,0x2b,0x35,0x2f,0x09,0x43,0x0c,
   0x6b,0x28,0x5b,0x39,0x27,0x0d,0x2b,0x39,0x27,
   0x11,0x27,0x3d,0x27,0x0d,0x43,0x08,0x73,0x24,
   0x57,0x41,0x1f,0x11,0x27,0x41,0x1f,0x15,0x27,
   0x41,0x1f,0x11,0x43,0x08,0x73,0x24,0x57,0x49,
   0x13,0x15,0x27,0x49,0x13,0x19,0x23,0x49,0x17,
   0x15,0x3f,0x0c,0x73,0x28,0x4f,0x15,0x13,0x25,
   0x07,0x1d,0x23,0x15,0x13,0x29,0x07,0x19,0x27,
   0x15,0x13,0x25,0x07,0x1d,0x3f,0x08,0x77,0x28,
   0x4f,0x11,0x1f,0x41,0x27,0x11,0x1f,0x41,0x27,
   0x15,0x1b,0x45,0x43,0x08,0x7b,0x24,0x4f,0x0d,
   0x27,0x39,0x2b,0x0d,0x27,0x3d,0x27,0x11,0x27,
   0x39,0x43,0x0c,0x7b,0x28,0x4b,0x0d,0x2b,0x35,
   0x2b,0x0d,0x2b,0x35,0x2b,0x0d,0x2f,0x35,0x43,
   0x08,0x7f,0x28,0x4b,0x09,0x33,0x2d,0x2f,0x09,
   0x33,0x2d,0x2f,0x09,0x37,0x2d,0x43,0x0c,0x7f,
   0x28,0x4b,0x05,0x3f,0x21,0x33,0x05,0x3f,0x25,
   0x2f,0x09,0x3b,0x25,0x47,0x08,0x87,0x24,0x4b,
   0x01,0x47,0x19,0x37,0x01,0x47,0x1d,0x33,0x05,
   0x43,0x1d,0x4b,0x08,0x87,0x28,0x47,0x01,0x4b,
   0x11,0x3b,0x01,0x4b,0x11,0x3b,0x01,0x4f,0x11,
   0x4b,0x0c,0x87,0x28,0xff,0x87,0x01,0xaf,0x08,
   0x8f,0x40,0xff,0x6b,0x01,0xaf,0x08,0x8f,0x40,
   0xff,0xff,0xff,0xff,0xff,0xff,0xff,0xff,0xff,
   0xff,0xff,0xff,0xff,0xff,0xff,0xff,0xff,0xff,
   0xff,0xff,0x07,0x00,
};

static UBYTE *aweblogodata;
static void *loadreqobj;
static struct Window *loadreqwin;
static struct Gadget *loadreqmsg,*loadreqfg;
static long loadreqmin,loadreqmax;
static struct TextAttr loadreqfont={0};

/*-----------------------------------------------------------------------*/

static void Setgadgetattrs(struct Gadget *gad,struct Window *win,struct Requester *req,...)
{  struct TagItem *tags=(struct TagItem *)((ULONG *)&req+1);
   if(SetGadgetAttrsA(gad,win,req,tags)) RefreshGList(gad,win,req,1);
}

static void Unpacklogo(void)
{  long l=5+((short *)logopacked)[0]*((short *)logopacked)[1];  /* 4 bytes size, 1 byte pad */
   UBYTE *p,*q,*end,c;
   short i,n;
   if(aweblogodata=AllocVec(l,MEMF_PUBLIC))
   {  end=logopacked+sizeof(logopacked);
      p=logopacked;
      q=aweblogodata;
      for(i=0;i<4;i++) *q++=*p++;
      for(;p<end;p++)
      {  n=(*p>>2)+1;
         c=(*p)&0x03;
         for(i=0;i<n;i++)
         {  *q++=c;
         }
      }
   }
}

static __asm __saveds void Nobackfillhook(void)
{
}

__asm __saveds void doStartupopen(register __a0 struct Screen *screen,
   register __a1 UBYTE *version)
{  struct Hook nobfhook;
   nobfhook.h_Entry=(HOOKFUNC)Nobackfillhook;
   if(screen)
   {  loadreqfont.ta_Name=screen->RastPort.Font->ln_Name;
      loadreqfont.ta_YSize=screen->RastPort.Font->tf_YSize;
   }
   Unpacklogo();
   if(aweblogodata)
   {  loadreqobj=WindowObject,
         WA_PubScreen,screen,
         WINDOW_Position,WPOS_CENTERSCREEN,
         WINDOW_Layout,VLayoutObject,
            LAYOUT_SpaceOuter,TRUE,
            LAYOUT_LeftSpacing,4,
            LAYOUT_RightSpacing,4,
            LAYOUT_BottomSpacing,4,
            CLASSACT_BackFill,&nobfhook,
            StartMember,HLayoutObject,
               LAYOUT_VertAlignment,LALIGN_CENTER,
               LAYOUT_HorizAlignment,LALIGN_LEFT,
               StartMember,HLayoutObject,
                  LAYOUT_BevelStyle,BVS_GROUP,
                  StartImage,PenMapObject,
                     IA_Data,aweblogodata,
                     PENMAP_Palette,aweblogopalette,
                     PENMAP_Screen,screen,
                  EndImage,
                  CHILD_MinWidth,192,
                  CHILD_MaxWidth,192,
                  CHILD_MinHeight,93,
               EndMember,
               StartImage,LabelObject,
                  IA_Font,&loadreqfont,
                  LABEL_Justification,LJ_CENTER,
                  LABEL_Text,"AWeb-II\n",
                  LABEL_Text,version,
               EndImage,
            EndMember,
            CHILD_WeightedWidth,0,
            StartMember,loadreqmsg=ButtonObject,
               GA_ReadOnly,TRUE,
               GA_Text,"",
               BUTTON_BevelStyle,BVS_NONE,
               BUTTON_DomainString,"AAAAAAAAAAAAAAAAAAAAAAAAAAAAAA",
            EndMember,
            StartMember,loadreqfg=FuelGaugeObject,
               FUELGAUGE_Min,0,
               FUELGAUGE_Max,100,
               FUELGAUGE_Level,0,
               FUELGAUGE_Ticks,0,
               FUELGAUGE_Percent,FALSE,
            EndMember,
         End,
      End;
      if(loadreqobj)
      {  loadreqwin=CA_OpenWindow(loadreqobj);
      }
   }
}

__asm __saveds void doStartupclose(void)
{  if(loadreqobj) DisposeObject(loadreqobj);
   loadreqobj=NULL;
   loadreqwin=NULL;
   if(aweblogodata)
   {  FreeVec(aweblogodata);
      aweblogodata=NULL;
   }
}

__asm __saveds void doStartupstate(register __d0 ULONG state)
{  UBYTE *msg=NULL;
   if(loadreqwin)
   {  switch(state)
      {  case LRQ_IMAGES:
            msg=AWEBSTR(MSG_STARTUP_IMAGES);
            loadreqmin=0;
            loadreqmax=25;
            break;
         case LRQ_FONTS:
            msg=AWEBSTR(MSG_STARTUP_FONTS);
            loadreqmin=25;
            loadreqmax=50;
            break;
         case LRQ_CACHE:
            msg=AWEBSTR(MSG_STARTUP_CACHE);
            loadreqmin=50;
            loadreqmax=100;
            break;
      }
      if(msg)
      {  Setgadgetattrs(loadreqmsg,loadreqwin,NULL,GA_Text,msg,TAG_END);
         Setgadgetattrs(loadreqfg,loadreqwin,NULL,FUELGAUGE_Level,loadreqmin,TAG_END);
      }
   }
}

__asm __saveds void doStartuplevel(register __d0 long ready,register __d1 long total)
{  long level;
   if(loadreqwin)
   {  level=ready*(loadreqmax-loadreqmin)/total+loadreqmin;
      Setgadgetattrs(loadreqfg,loadreqwin,NULL,FUELGAUGE_Level,level,TAG_END);
   }
}


