/*  Part of XPCE --- The SWI-Prolog GUI toolkit

    Author:        Jan Wielemaker and Anjo Anjewierden
    E-mail:        jan@swi-prolog.org
    WWW:           http://www.swi.psy.uva.nl/projects/xpce/
    Copyright (c)  1985-2025, University of Amsterdam
			      SWI-Prolog Solutions b.v.
    All rights reserved.

    Redistribution and use in source and binary forms, with or without
    modification, are permitted provided that the following conditions
    are met:

    1. Redistributions of source code must retain the above copyright
       notice, this list of conditions and the following disclaimer.

    2. Redistributions in binary form must reproduce the above copyright
       notice, this list of conditions and the following disclaimer in
       the documentation and/or other materials provided with the
       distribution.

    THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS
    "AS IS" AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT
    LIMITED TO, THE IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS
    FOR A PARTICULAR PURPOSE ARE DISCLAIMED. IN NO EVENT SHALL THE
    COPYRIGHT OWNER OR CONTRIBUTORS BE LIABLE FOR ANY DIRECT, INDIRECT,
    INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING,
    BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES;
    LOSS OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER
    CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT
    LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN
    ANY WAY OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF THE
    POSSIBILITY OF SUCH DAMAGE.
*/

#include <h/kernel.h>
#include <h/graphics.h>
#include "include.h"

#ifdef HAVE_X11_EXTENSIONS_XRANDR_H
#ifdef HAVE_LIBXRANDR
#define Connection XRRConnection
#include <X11/extensions/Xrandr.h>
#undef Connection
#else
#undef HAVE_X11_EXTENSIONS_XRANDR_H
#endif
#endif

#define X11LastEventTime() ((Time)LastEventTime())
#define utf8_get_uchar(s, chr) (unsigned char*)utf8_get_char((char *)(s), chr)

static XrmOptionDescRec opTable[] =
{ {"-xrm",	NULL,	XrmoptionResArg, NULL }
};


void
ws_flush_display(DisplayObj d)
{ DisplayWsXref r = d->ws_ref;

  XFlush(r->display_xref);
}


/* - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
Seems this can get into a loop. Why? For   now  we will jump out after a
while. Seems processing 1000 messages is   more  then enough to fullfill
the aim of this function.

Maybe  we  should  somehow   check   the    state   of   the  connecting
file-descriptor to see whether the server is lost?
- - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - */

void
ws_synchronise_display(DisplayObj d)
{ DisplayWsXref r = d->ws_ref;
  static int retry=0;
  int i;

  XFlush(r->display_xref);
  XSync(r->display_xref, False);


  for(i=1000; (XtAppPending(pceXtAppContext(NULL)) & XtIMAll) && --i > 0; )
    XtAppProcessEvent(pceXtAppContext(NULL), XtIMAll);

  if ( i == 0 )
  { Cprintf("ws_synchronise_display(): looping??\n");

    if ( ++retry == 10 )
    { Cprintf("Trouble, trying to abort\n");
      hostAction(HOST_ABORT);
    } else if ( retry == 20 )
    { Cprintf("Serious trouble, calling exit()\n");
      exit(1);
    }
  } else
    retry = 0;				/* seems to work again */
}


void
ws_bell_display(DisplayObj d, int volume)
{ DisplayWsXref r = d->ws_ref;

  XBell(r->display_xref, volume);
}


void
ws_get_size_display(DisplayObj d, int *w, int *h)
{ DisplayWsXref r = d->ws_ref;
  int screen;

  screen = XDefaultScreen(r->display_xref);
  *w = XDisplayWidth(r->display_xref,  screen);
  *h = XDisplayHeight(r->display_xref, screen);
}


Name
ws_get_visual_type_display(DisplayObj d)
{ if ( ws_depth_display(d) == 1 )
    return NAME_monochrome;
  else
  { DisplayWsXref r = d->ws_ref;
    Visual *v = XDefaultVisual(r->display_xref,
			       DefaultScreen(r->display_xref));
    int vclass = v->class;

    switch(vclass)
    { case StaticGray:	return NAME_staticGrey;
      case GrayScale:	return NAME_greyScale;
      case StaticColor:	return NAME_staticColour;
      case PseudoColor:	return NAME_pseudoColour;
      case TrueColor:	return NAME_trueColour;
      case DirectColor:	return NAME_directColour;
      default:		return (Name) toInt(vclass);
    }
  }
}


int
ws_depth_display(DisplayObj d)
{ DisplayWsXref r = d->ws_ref;

  return r->depth;
}


int
ws_resolution_display(DisplayObj d, int *rx, int *ry)
{ DisplayWsXref r = d->ws_ref;
  int screen, wpx, wmm, hpx, hmm;

  screen = XDefaultScreen(r->display_xref);

#ifdef HAVE_X11_EXTENSIONS_XRANDR_H
  int nsizes = 0;
  XRRScreenSize *ss = XRRSizes(r->display_xref,  screen, &nsizes);

  if ( nsizes > 0 )
  { DEBUG(NAME_scale, Cprintf("Getting dimensions from Xrandr extension\n"));
    wpx = ss[0].width;
    hpx = ss[0].height;
    wmm = ss[0].mwidth;
    hmm = ss[0].mheight;
  } else
#endif
  { DEBUG(NAME_scale, Cprintf("Getting dimensions from Xlib\n"));
    wpx = XDisplayWidth(r->display_xref,  screen);
    hpx = XDisplayHeight(r->display_xref, screen);
    wmm = XDisplayWidthMM(r->display_xref,  screen);
    hmm = XDisplayHeightMM(r->display_xref, screen);
  }

  DEBUG(NAME_scale, Cprintf(" --> %dx%d pixels, %dx%dmm\n",
			    wpx, hpx, wmm, hmm));

  if ( wmm && hmm )
  { int xdpi = (int)((double)wpx*25.4 / (double)wmm + 0.5);
    int ydpi = (int)((double)hpx*25.4 / (double)hmm + 0.5);

    double ratio = (double)xdpi/(double)ydpi;
    if ( ratio > 0.9 && ratio < 1.1 )
    { *rx = xdpi;
      *ry = ydpi;
      succeed;
    }
  }

  DEBUG(NAME_scale, Cprintf("Dimensions seem bogus.  Assuming 100x100DPI\n"));
  *rx = *ry = 100;

  succeed;
}


void
ws_activate_screen_saver(DisplayObj d)
{ DisplayWsXref r = d->ws_ref;

  XForceScreenSaver(r->display_xref, ScreenSaverActive);
}


void
ws_deactivate_screen_saver(DisplayObj d)
{ DisplayWsXref r = d->ws_ref;

  XForceScreenSaver(r->display_xref, ScreenSaverReset);
}


void
ws_init_display(DisplayObj d)
{ DisplayWsXref ref = alloc(sizeof(display_ws_ref));

  memset(ref, 0, sizeof(*ref));
  ref->depth		= 1;
  ref->black_pixel      = 1L;
  ref->foreground_pixel = ref->black_pixel;
  ref->background_pixel = ref->white_pixel;

  d->ws_ref = ref;
}


const char *
skipint(const char *s)
{ const char *s0 = s;

  while ( *s && isdigit(*s) )
    s++;

  return s > s0 ? s : NULL;
}

status
ws_legal_display_name(const char *s)
{ while( *s && (isalnum(*s) || *s == '.') )
    s++;
  if ( *s != ':' )
    fail;
  s++;
  if ( !(s = skipint(s)) )
    fail;
  if ( *s == '.' )
  { s++;
    if ( !(s = skipint(s)) )
      fail;
  }

  return *s == '\0';
}


status
ws_opened_display(DisplayObj d)
{ return display_x11_ref(d, display_xref) == NULL ? FAIL : SUCCEED;
}


/* - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
We'd like to keep the XPCE resource  file `Pce' in XPCE's homedirectory,
so we can have multiple versions of this   file. For this reason we push
$XAPPLRESDIR.

I've diagnosed that actually, the primary path searched is not the first
element of this path, but the  last!?  If   anyone  can  point me to the
proper  semantics  of  $XAPPLRESDIR.   grep  through  /usr/man/man3/X*.3
doesn't give any hints ...
- - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - */

void
ws_open_display(DisplayObj d)
{ DisplayWsXref ref = d->ws_ref;
  char *address;
  Display *display;

  char **PCEargv = malloc(sizeof(char**)*10);
  PCEargv[0] = "pl";
  PCEargv[1] = NULL;
  PCEargc = 1;

  address = isDefault(d->address) ? NULL : strName(d->address);
  display = XtOpenDisplay(pceXtAppContext(NULL),
			  address, "xpce",
			  "Pce",	/* resource class (not used) */
			  opTable, XtNumber(opTable),
			  &PCEargc, PCEargv);

  if ( !display )
  { char problem[LINESIZE];
    char *theaddress = XDisplayName(address);

    if ( isDefault(d->address) && !getenv("DISPLAY") )
      strcpy(problem, "no DISPLAY environment variable");
    else if ( !ws_legal_display_name(theaddress) )
      snprintf(problem, sizeof(problem), "malformed DISPLAY address: %s",
	       theaddress);
    else
      strcpy(problem, "No permission to contact X-server?");

    errorPce(d, NAME_noXServer,
	     CtoName(theaddress), CtoString(problem), 0);
    return;
  } else
  { int screen = DefaultScreen(display);

    DEBUG(NAME_display,
	  XSynchronize(display, True));

    ref->display_xref = display;
    ref->screen	      = screen;
    ref->visual	      = DefaultVisual(display, screen);
    ref->colour_map   = DefaultColormap(display, screen);
    ref->white_pixel  = WhitePixel(display, screen);
    ref->black_pixel  = BlackPixel(display, screen);
    ref->depth        = DefaultDepth(display, screen);

#ifdef O_XIM
    if ( !(ref->im = XOpenIM(display, NULL, NULL, NULL)) )
    { DEBUG(NAME_event, Cprintf("Could not open XIM\n"));
    }
#endif
  }

  { Arg args[5];
    Cardinal n = 0;

    XtSetArg(args[n], XtNmappedWhenManaged, False); n++;
    XtSetArg(args[n], XtNwidth,		    64);    n++;
    XtSetArg(args[n], XtNheight,	    64);    n++;

    ref->shell_xref = XtAppCreateShell("xpce",
				       "Pce", /* Resource Class */
				       applicationShellWidgetClass,
				       display,
				       args, n);
  }

  if ( !ref->shell_xref )
  { errorPce(d, NAME_noMainWindow);
    return;
  }

  XtRealizeWidget(ref->shell_xref);	/* Need a window for GC */

  ref->root_bitmap	= XCreatePixmap(display, XtWindow(ref->shell_xref),
					8, 4, 1);
}


void
ws_quit_display(DisplayObj d)
{ Cprintf("Cannot close display yet\n");
}


static DrawContext
new_draw_context(DisplayObj d, Drawable drawable, Name kind)
{ DrawContext ctx = alloc(sizeof(struct draw_context));
  DisplayWsXref r = d->ws_ref;
  Display *display = r->display_xref;
  XGCValues values;
  unsigned long black, white;
  Name vclass = ws_get_visual_type_display(d);

  memset(ctx, 0, sizeof(*ctx));

# define GCALL (GCFunction|GCForeground|GCBackground|GCGraphicsExposures)

  ctx->kind = kind;

  if ( kind == NAME_bitmap )
  { values.foreground = 1;
    values.background = 0;
    black = 1;
    white = 0;
    ctx->depth = 1;
  } else
  { values.foreground = r->foreground_pixel;
    values.background = r->background_pixel;
    black = r->black_pixel;
    white = r->white_pixel;
    ctx->depth = r->depth;
  }

  values.graphics_exposures = False;

  values.function   = GXinvert;			/* On true- and direct-color */
  if ( vclass == NAME_trueColour ||		/* displays, invert value */
       vclass == NAME_directColour )		/* On mapped displays */
    values.plane_mask = ~0L;			/* 0 and 1 are normally  */
  else						/* blank and white */
    values.plane_mask = 1;
  ctx->complementGC = XCreateGC(display, drawable, GCALL|GCPlaneMask, &values);

  values.function   = GXcopy;
  values.fill_rule  = EvenOddRule;
  values.arc_mode   = ArcPieSlice;
  ctx->fillGC	    = XCreateGC(display, drawable,
				GCALL|GCFillRule|GCArcMode, &values);

  values.fill_style = FillOpaqueStippled;
  ctx->bitmapGC	    = XCreateGC(display, drawable,
				GCALL|GCFillStyle|GCFillRule, &values);

  values.function   = (black == 0L ? GXor : GXand);
  ctx->andGC	    = XCreateGC(display, drawable,
				GCALL|GCFillRule|GCArcMode, &values);

  values.function   = GXcopy;
  ctx->workGC	    = XCreateGC(display, drawable, GCALL, &values);
  ctx->copyGC	    = XCreateGC(display, drawable, GCALL, &values);
  ctx->opGC	    = XCreateGC(display, drawable, GCALL, &values);

  values.foreground = values.background;
  ctx->clearGC	    = XCreateGC(display, drawable, GCALL, &values);

  values.foreground = black;
  ctx->shadowGC	    = XCreateGC(display, drawable, GCALL, &values);
  values.foreground = white;
  ctx->reliefGC	    = XCreateGC(display, drawable, GCALL, &values);

#undef GCALL

  ctx->pen		= -1;
  ctx->dash		= NAME_none;
  ctx->fill		= NIL;
  ctx->arcmode		= NAME_pieSlice;
  ctx->and_pattern      = NIL;
  ctx->font		= NIL;
  ctx->colour		= NIL;
  ctx->background       = NIL;
  ctx->foreground_pixel = 0L;
  ctx->background_pixel = 0L;
  ctx->subwindow_mode   = OFF;
  ctx->invert_mode      = OFF;

  return ctx;
}


status
ws_init_graphics_display(DisplayObj d)
{ DisplayWsXref r = d->ws_ref;

  if ( r->pixmap_context == NULL )
  { r->bitmap_context   = new_draw_context(d,
					   r->root_bitmap,
					   NAME_bitmap);
    r->pixmap_context   = new_draw_context(d,
					   XtWindow(r->shell_xref),
					   NAME_pixmap);

  }

  succeed;
}


void
ws_foreground_display(DisplayObj d, Colour c)
{ DisplayWsXref r = d->ws_ref;

  r->foreground_pixel = getPixelColour(c, d);
}


void
ws_background_display(DisplayObj d, Colour c)
{ DisplayWsXref r = d->ws_ref;

  r->background_pixel = getPixelColour(c, d);
}


void
ws_draw_in_display(DisplayObj d, Graphical gr, BoolObj invert, BoolObj subtoo)
{ d_screen(d);
  if ( invert == ON ) r_invert_mode(ON);
  if ( subtoo == ON ) r_subwindow_mode(ON);
  RedrawArea(gr, gr->area);
  r_invert_mode(OFF);
  r_subwindow_mode(OFF);
  d_done();
}


void
ws_grab_server(DisplayObj d)
{ DisplayWsXref r = d->ws_ref;

  XGrabServer(r->display_xref);
}


void
ws_ungrab_server(DisplayObj d)
{ DisplayWsXref r = d->ws_ref;

  XUngrabServer(r->display_xref);
}


Int
ws_display_connection_number(DisplayObj d)
{ DisplayWsXref r = d->ws_ref;

  if ( r && r->display_xref )
    return toInt(ConnectionNumber(r->display_xref));
  else
    fail;
}


status
ws_events_queued_display(DisplayObj d)
{ DisplayWsXref r = d->ws_ref;

  if ( r && r->display_xref )
  { XSync(r->display_xref, False);
    if ( XtAppPending(pceXtAppContext(NULL)) & XtIMAll )
      succeed;
  }

  fail;
}


status
ws_pointer_location_display(DisplayObj d, int *x, int *y)
{ DisplayWsXref r = d->ws_ref;
  XWindowAttributes atts;
  Window rr, cr;
  int wx, wy;
  unsigned int mask;

  XGetWindowAttributes(r->display_xref, XtWindow(r->shell_xref), &atts);
  if ( XQueryPointer(r->display_xref, atts.root, &rr, &cr,
		     x, y, &wx, &wy, &mask) )
  { succeed;
  }

  fail;
}



#ifdef HAVE_X11_EXTENSIONS_XINERAMA_H
#ifdef HAVE_LIBXINERAMA
#include <X11/extensions/Xinerama.h>
#else
#undef HAVE_X11_EXTENSIONS_XINERAMA_H
#endif
#endif

status
ws_init_monitors_display(DisplayObj d)
{
#ifdef HAVE_X11_EXTENSIONS_XINERAMA_H
  DisplayWsXref r = d->ws_ref;
  XineramaScreenInfo *screens;
  int count;

  if ( r && r->display_xref &&
       XineramaIsActive(r->display_xref) &&
       (screens = XineramaQueryScreens(r->display_xref, &count)) )
  { int i;

    assign(d, monitors, newObject(ClassChain, EAV));
    for(i=0; i<count; i++)
    { appendChain(d->monitors,
		  newObject(ClassMonitor, toInt(screens[i].screen_number),
			    newObject(ClassArea,
				      toInt(screens[i].x_org),
				      toInt(screens[i].y_org),
				      toInt(screens[i].width),
				      toInt(screens[i].height), EAV), EAV));
    }

    XFree(screens);
  } else
#endif
  { Size sz = getSizeDisplay(d);

    if ( sz )
    { assign(d, monitors, newObject(ClassChain, EAV));
      appendChain(d->monitors,
		  newObject(ClassMonitor, toInt(0),
			    newObject(ClassArea, toInt(0), toInt(0),
				      sz->w, sz->h, EAV), EAV));
    }
  }

  succeed;
}


		 /*******************************
		 *	    CUT-BUFFER		*
		 *******************************/

status
ws_set_cutbuffer(DisplayObj d, int n, PceString s)
{ DisplayWsXref r = d->ws_ref;

  if ( n == 0 )
    XStoreBytes(r->display_xref, (char*)s->s_text, str_datasize(s));
  else
    XStoreBuffer(r->display_xref, (char*)s->s_text, str_datasize(s), n);

  succeed;
}


StringObj
ws_get_cutbuffer(DisplayObj d, int n)
{ DisplayWsXref r = d->ws_ref;
  char *data;
  int size;
  string str;
  StringObj rval;

  if ( n == 0 )
    data = XFetchBytes(r->display_xref, &size);
  else
    data = XFetchBuffer(r->display_xref, &size, valInt(n));

  if ( str_set_n_ascii(&str, size, data) )
    rval = StringToString(&str);
  else
    rval = FAIL;

  XFree(data);
  answer(rval);
}

		 /*******************************
		 *	     SELECTION		*
		 *******************************/

/* See http://www.jwz.org/doc/x-cut-and-paste.html
*/

static int	 selection_complete;
static StringObj selection_value;
static Name	 selection_error;
static Atom	 XA_UTF8_STRING;

static Atom
nameToSelectionAtom(DisplayObj d, Name name)
{ if ( name == NAME_primary )	return XA_PRIMARY;
  if ( name == NAME_secondary ) return XA_SECONDARY;
  if ( name == NAME_string )	return XA_STRING;
  if ( name == NAME_utf8_string )
  { if ( !XA_UTF8_STRING )
      XA_UTF8_STRING = DisplayAtom(d, getv(name, NAME_upcase, 0, NULL));
    return XA_UTF8_STRING;
  }

  return DisplayAtom(d, getv(name, NAME_upcase, 0, NULL));
}


static Name
atomToSelectionName(DisplayObj d, Atom a)
{ if ( a == XA_PRIMARY )     return NAME_primary;
  if ( a == XA_SECONDARY )   return NAME_secondary;
  if ( a == XA_STRING )      return NAME_string;
  if ( a == XA_UTF8_STRING ) return NAME_utf8_string;

  { Name xname = CtoName(DisplayAtomToString(d, a));
    Name lname = getv(xname, NAME_downcase, 0, NULL);

    return CtoKeyword(strName(lname));
  }

  fail;
}


unsigned long
ws_get_selection_timeout()
{ return XtAppGetSelectionTimeout(pceXtAppContext(NULL));
}


void
ws_set_selection_timeout(unsigned long time)
{ XtAppSetSelectionTimeout(pceXtAppContext(NULL), time);
}


static void
collect_selection_display(Widget w, XtPointer xtp,
			  Atom *selection, Atom *type,
			  XtPointer value, unsigned long *len, int *format)
{ DisplayObj d = xtp;
  string s;

  DEBUG(NAME_selection,
	Cprintf("collect_selection_display(): sel=%s; type=%s; "
		"format=%d; len=%ld\n",
		DisplayAtomToString(d, *selection),
		DisplayAtomToString(d, *type),
		*format, *len));

  if ( *type == XT_CONVERT_FAIL || *type == (Atom)0 )
  { selection_error = CtoName("Selection conversion failed");
    selection_complete = TRUE;
    return;
  } else if ( *type == XA_STRING )
  { if ( *format == 8 )
    { if ( !str_set_n_ascii(&s, *len, (char *)value) )
      { selection_error = CtoName("PceString too long");
	selection_complete = TRUE;
	return;
      }
    } else
    { selection_error = CtoName("Bad format");
      selection_complete = TRUE;
      return;
    }

    selection_value = StringToString(&s);
    XtFree(value);
  } else if ( *type == XA_UTF8_STRING )
  { if ( *format == 8 )
    { unsigned long l = *len;
      charA *bufA, *outA;
      const charA *in = value;
      const charA *end = &in[*len];
      int chr;

      if ( l > 8*1024*2024 )		/* sanity check */
      { selection_error = CtoName("Selection too long");
	selection_complete = TRUE;
	return;
      }

      outA = bufA = pceMalloc(l);
      int iso_latin_1 = TRUE;
      while(in<end)
      { in = utf8_get_uchar(in, &chr);

	if ( chr <= 0xff )
	{ *outA++ = chr;
	} else
	{ iso_latin_1 = FALSE;
	  break;
	}
      }

      if ( iso_latin_1 )
      { str_set_n_ascii(&s, outA-bufA, (char*)bufA);
	selection_value = StringToString(&s);
	pceFree(bufA);
      } else
      { charW *bufW = pceRealloc(bufA, l*sizeof(charW));
	charW *out = bufW;

	for(in = value; in<end; )
	{ in = utf8_get_uchar(in, &chr);

	  *out++ = chr;
	}

	str_set_n_wchar(&s, out-bufW, bufW);
	selection_value = StringToString(&s);
	pceFree(bufW);
      }
    } else
    { selection_error = CtoName("UTF8_STRING Selection: bad format");
    }

    XtFree(value);
  } else if ( *type == XT_CONVERT_FAIL )
  { selection_error = NAME_timeout;
  } else
  { char buf[256];
    char *s = DisplayAtomToString(d, *type);

    DEBUG(NAME_selection, Cprintf("Bad type: Atom %d (%s)\n", *type, s));
    sprintf(buf, "Bad type: %s", s);

    selection_error = CtoName(buf);
  }

  selection_complete = TRUE;
}


Any
ws_get_selection(DisplayObj d, Name which, Name target)
{ DisplayWsXref r = d->ws_ref;

  if ( target == NAME_text )
    target = NAME_utf8_string;

  DEBUG(NAME_selection, Cprintf("ws_get_selection(d,%s,%s)\n",
				strName(which), strName(target)));

  selection_complete = FALSE;
  selection_error = NIL;
  selection_value = NIL;
  XtGetSelectionValue(r->shell_xref,
		      nameToSelectionAtom(d, which),
		      nameToSelectionAtom(d, target),
		      collect_selection_display,
		      d,
		      X11LastEventTime());

  for(int i=0; i<10 && !selection_complete; i++)
  { dispatchDisplayManager(d->display_manager, DEFAULT, toInt(50));

    if ( notNil(selection_error) )
    { errorPce(d, NAME_getSelection, which, selection_error);
      fail;
    }
  }

  answer(selection_value);
}


static DisplayObj
widgetToDisplay(Widget w)
{ DisplayManager dm = TheDisplayManager();
  Cell cell;

  for_cell(cell, dm->members)
  { DisplayObj d = cell->value;
    DisplayWsXref r = d->ws_ref;

    if ( r->shell_xref == w )
      return d;
  }

  fail;
}


/* - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
Deliver the selection. This needs  to  be   expanded  a  bit.  We should
provide more answers to the `targets'  request, the protocol requires us
to implement MULTIPLE as well and we should provide COMPOUND_TEXT and/or
UTF8_STRING as return values for text.

Full  specs  for  the  inter-client  communication    can  be  found  at
http://tronche.com/gui/x/icccm/ (Inter-Client Communication  Conventions
Manual)
- - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - */

static Boolean
convert_selection_display(Widget w,
			  Atom *selection, Atom *target, Atom *type_return,
			  XtPointer *value, unsigned long *len, int *format)
{ DisplayObj d = widgetToDisplay(w);
  Hyper h;
  Function msg;
  Name which = atomToSelectionName(d, *selection);
  Name hypername = getAppendName(which, NAME_selectionOwner);
  DisplayWsXref r = d->ws_ref;

  DEBUG(NAME_selection,
	Cprintf("Request for %s selection\n", pp(which)));

  if ( d &&
       (h = getFindHyperObject(d, hypername, DEFAULT)) &&
       (msg = getAttributeObject(h, NAME_convertFunction)) &&
       (msg = checkType(msg, TypeFunction, NIL)) )
  { CharArray ca;
    Name tname = atomToSelectionName(d, *target);

    DEBUG(NAME_selection,
	  Cprintf("\ttarget = %s\n", pp(tname)));

    if ( tname == NAME_targets )
    { Atom *buf = (Atom *)XtMalloc(3*sizeof(Atom));
      int n = 0;

      buf[n++] = XInternAtom(r->display_xref, "TARGETS", False);
      buf[n++] = XA_STRING;
      buf[n++] = DisplayAtom(d, CtoName("UTF8_STRING"));
      *value = buf;
      *len = n;
      *format = 32;
      *type_return = XA_ATOM;

      return True;
    }

    if ( (ca = getForwardReceiverFunction(msg, h->to, which, tname, EAV)) &&
	 (ca = checkType(ca, TypeCharArray, NIL)) )
    { PceString s = &ca->data;
      static Name tplainutf8 = NULL;

      if ( !tplainutf8 )
	tplainutf8 = CtoName("text/plain;charset=utf-8");

      if ( tname == NAME_utf8_string || tname == tplainutf8 )
      { char *buf;
	int length;
	char *out;

	if ( isstrA(s) )
	  length = pce_utf8_enclenA((char*)s->s_textA, s->s_size);
	else
	  length = pce_utf8_enclenW(s->s_textW, s->s_size);

	DEBUG(NAME_selection,
	      Cprintf("\tRequest for UTF-8.  Found %d chars in selection\n",
		      length));

	out = buf = XtMalloc(length+1);
	if ( isstrA(s) )
	{ const charA *f = s->s_textA;
	  const charA *e = &f[s->s_size];

	  for(; f<e; f++)
	    out = utf8_put_char(out, *f);
	  *out = EOS;
	} else
	{ const charW *f = s->s_textW;
	  const charW *e = &f[s->s_size];

	  for(; f<e; f++)
	    out = utf8_put_char(out, *f);
	  *out = EOS;
	}
	assert(out == buf+length);

	*value = buf;
	*len = length;
	*format = 8;
	*type_return = DisplayAtom(d, CtoName("UTF8_STRING"));
      } else
      { int data = str_datasize(s);
	char *buf = XtMalloc(data);
	int fmt = (isstrA(s) ? sizeof(charA) : sizeof(charW)) * 8;

	DEBUG(NAME_selection,
	      Cprintf("returning XA_STRING, %d characters format = %d\n",
		      data, fmt));

	memcpy(buf, s->s_text, data);
	*value = buf;
	*len = data;
	*format = fmt;
	*type_return = XA_STRING;
      }

      return True;
    }
  }

  return False;
}


static void
loose_selection_widget(Widget w, Atom *selection)
{ DisplayObj d = widgetToDisplay(w);

  DEBUG(NAME_selection,
	Cprintf("%s: Loosing %s selection",
		pp(d), pp(atomToSelectionName(d, *selection))));

  if ( d )
    looseSelectionDisplay(d, atomToSelectionName(d, *selection));
}


void
ws_disown_selection(DisplayObj d, Name selection)
{ DisplayWsXref r = d->ws_ref;

  XtDisownSelection(r->shell_xref,
		    nameToSelectionAtom(d, selection),
		    X11LastEventTime());
}


status
ws_own_selection(DisplayObj d, Name selection, Name type)
{ DisplayWsXref r = d->ws_ref;

  if ( XtOwnSelection(r->shell_xref, nameToSelectionAtom(d, selection),
		      X11LastEventTime(),
		      convert_selection_display,
		      loose_selection_widget,
		      NULL) )
    succeed;

  fail;
}


Name
ws_window_manager(DisplayObj d)
{
#if O_MOTIF
  DisplayWsXref r = d->ws_ref;

  if ( XmIsMotifWMRunning(r->shell_xref) )
    return NAME_mwm;
#endif

  fail;
}


void
ws_synchronous(DisplayObj d)
{ DisplayWsXref r = d->ws_ref;

  XSynchronize(r->display_xref, True);
}


void
ws_asynchronous(DisplayObj d)
{ DisplayWsXref r = d->ws_ref;

  XSynchronize(r->display_xref, False);
}

		 /*******************************
		 *	    POSTSCRIPT		*
		 *******************************/

static int
psdepthXImage(XImage *im)
{ if ( im->depth < 3 )			/* 1, 2 */
    return im->depth;
  if ( im->depth < 8 )
    return 4;
  return 8;
}


status
ws_postscript_display(DisplayObj d, int iscolor)
{ XWindowAttributes atts;
  XImage *im;
  int iw, ih;
  Window root;
  DisplayWsXref r;

  openDisplay(d);
  r = d->ws_ref;

  XGetWindowAttributes(r->display_xref, XtWindow(r->shell_xref), &atts);
  root = atts.root;
  XGetWindowAttributes(r->display_xref, root, &atts);

  iw = atts.width; ih = atts.height;
  im = XGetImage(r->display_xref, atts.root,
		 0, 0, iw, ih, AllPlanes, ZPixmap);

  ps_output("0 0 ~D ~D ~D ~N\n", iw, ih,
	    psdepthXImage(im),
	    iscolor ? NAME_rgbimage : NAME_greymap);
  postscriptXImage(im, NULL, 0, 0, iw, ih, r->display_xref, r->colour_map,
		   0, iscolor);
  ps_output("\n");

  XDestroyImage(im);
  succeed;
}


Image
ws_grab_image_display(DisplayObj d, int x, int y, int width, int height)
{ XWindowAttributes atts;
  XImage *im = NULL;
  Image i = NULL;
  Window root;
  DisplayWsXref r;

  openDisplay(d);
  r = d->ws_ref;

				/* display attributes */
  XGetWindowAttributes(r->display_xref, XtWindow(r->shell_xref), &atts);
  root = atts.root;
  XGetWindowAttributes(r->display_xref, root, &atts);

  if ( x < 0 )			/* clip to display size */
  { width += x;
    x = 0;
  }
  if ( y < 0 )
  { height += y;
    y = 0;
  }
  if ( x + width > atts.width )
    width = atts.width - x;
  if ( y + height > atts.height )
    height = atts.height - y;

  if ( width <= 0 || height <= 0 )
    fail;

  i = answerObject(ClassImage, NIL,
		   toInt(width), toInt(height), NAME_pixmap, EAV);
  im = XGetImage(r->display_xref, atts.root,
		 x, y, width, height, AllPlanes, ZPixmap);

  if ( i && im )
  { setXImageImage(i, im);
    assign(i, depth, toInt(im->depth));
  } else
  { if ( im )
      XDestroyImage(im);
    if ( i )
    { freeObject(i);
      i = NULL;
    }
  }

  answer(i);
}
