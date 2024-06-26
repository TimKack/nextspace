/*
 *  Workspace window manager
 *  Copyright (c) 2015-2021 Sergii Stoian
 *
 *  WINGs library (Window Maker)
 *  Copyright (c) 1998 scottc
 *  Copyright (c) 1999-2004 Dan Pascu
 *  Copyright (c) 1999-2000 Alfredo K. Kojima
 *  Copyright (c) 2014 Window Maker Team
 *
 *  This program is free software; you can redistribute it and/or modify
 *  it under the terms of the GNU General Public License as published by
 *  the Free Software Foundation; either version 2 of the License, or
 *  (at your option) any later version.
 *
 *  This program is distributed in the hope that it will be useful,
 *  but WITHOUT ANY WARRANTY; without even the implied warranty of
 *  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 *  GNU General Public License for more details.
 *
 *  You should have received a copy of the GNU General Public License along
 *  with this program; if not, write to the Free Software Foundation, Inc.,
 *  51 Franklin Street, Fifth Floor, Boston, MA 02110-1301 USA.
 */

#include <string.h>

#include "util.h"
#include "string_utils.h"

#include "wscreen.h"
#include "widgets.h"
#include "wevent.h"
#include "wcolor.h"
#include "drawing.h"
#include "wframe.h"

#define DEFAULT_RELIEF 	WRGroove
#define DEFAULT_TITLE_POSITION	WTPAtTop
#define DEFAULT_WIDTH		40
#define DEFAULT_HEIGHT		40

static void destroyFrame(WMFrame *fPtr);
static void paintFrame(WMFrame *fPtr);
static void repaintFrame(WMFrame *fPtr);

void WMSetFrameTitlePosition(WMFrame *fPtr, WMTitlePosition position)
{
  fPtr->flags.titlePosition = position;

  if (fPtr->view->flags.realized) {
    repaintFrame(fPtr);
  }
}

void WMSetFrameRelief(WMFrame *fPtr, WMReliefType relief)
{
  fPtr->flags.relief = relief;

  if (fPtr->view->flags.realized) {
    repaintFrame(fPtr);
  }
}

void WMSetFrameTitle(WMFrame *fPtr, const char *title)
{
  if (fPtr->caption)
    wfree(fPtr->caption);

  if (title)
    fPtr->caption = wstrdup(title);
  else
    fPtr->caption = NULL;

  if (fPtr->view->flags.realized) {
    repaintFrame(fPtr);
  }
}

static void repaintFrame(WMFrame *fPtr)
{
  WMView *view = fPtr->view;
  WMScreen *scrPtr = view->screen;

  XClearWindow(scrPtr->display, view->window);
  paintFrame(fPtr);
}

static void paintFrame(WMFrame *fPtr)
{
  WMView *view = fPtr->view;
  WMScreen *scrPtr = view->screen;
  WMFont *font = scrPtr->normalFont;
  Display *display = scrPtr->display;
  int tx, ty, tw, th, tlen;
  int fy, fh;
  Bool drawTitle;

  if (fPtr->caption != NULL) {
    th = WMFontHeight(font);
    tlen = strlen(fPtr->caption);
  } else {
    th = 0;
    tlen = 0;
  }

  switch (fPtr->flags.titlePosition) {
  case WTPAboveTop:
    ty = 0;
    fy = th + 4;
    fh = view->size.height - fy;
    break;

  case WTPAtTop:
    ty = 0;
    fy = th / 2;
    fh = view->size.height - fy;
    break;

  case WTPBelowTop:
    ty = 4;
    fy = 0;
    fh = view->size.height;
    break;

  case WTPAboveBottom:
    ty = view->size.height - th - 4;
    fy = 0;
    fh = view->size.height;
    break;

  case WTPAtBottom:
    ty = view->size.height - th;
    fy = 0;
    fh = view->size.height - th / 2;
    break;

  case WTPBelowBottom:
    ty = view->size.height - th;
    fy = 0;
    fh = view->size.height - th - 4;
    break;

  default:
    ty = 0;
    fy = 0;
    fh = view->size.height;
  }

  if (tlen > 0 && fPtr->flags.titlePosition != WTPNoTitle) {
    tw = WMWidthOfString(font, fPtr->caption, tlen);

    tx = (view->size.width - tw) / 2;

    drawTitle = True;
  } else {
    tw = tx = 0;
    drawTitle = False;
  }

  {
    XRectangle rect;
    Region region, tmp;
    GC gc[4];
    int i;

    region = XCreateRegion();

    rect.x = 0;
    rect.y = 0;
    rect.width = view->size.width;
    rect.height = view->size.height;
    XUnionRectWithRegion(&rect, region, region);
    if (drawTitle) {
      tmp = XCreateRegion();
      rect.x = tx;
      rect.y = ty;
      rect.width = tw;
      rect.height = th;
      XUnionRectWithRegion(&rect, tmp, tmp);
      XSubtractRegion(region, tmp, region);
      XDestroyRegion(tmp);
    }
    gc[0] = WMColorGC(scrPtr->black);
    gc[1] = WMColorGC(scrPtr->darkGray);
    gc[2] = WMColorGC(scrPtr->gray);
    gc[3] = WMColorGC(scrPtr->white);

    for (i = 0; i < 4; i++) {
      XSetRegion(display, gc[i], region);
    }
    XDestroyRegion(region);

    WMDrawReliefWithGC(scrPtr, view->window, 0, fy, view->size.width, fh,
                       fPtr->flags.relief, gc[0], gc[1], gc[2], gc[3]);

    for (i = 0; i < 4; i++) {
      XSetClipMask(display, gc[i], None);
    }
  }

  if (drawTitle) {
    /* can't draw AA text over and over again because it gets messed */
    /* // TODO create the dbl buffer pixmap when create/set frame title */
    if (scrPtr->antialiasedText) {
      Drawable d;

      d = XCreatePixmap(display, view->window, tw, th, scrPtr->depth);
      XFillRectangle(display, d, WMColorGC(view->backColor), 0, 0, tw, th);

      WMDrawString(scrPtr, d, fPtr->textColor ? fPtr->textColor : scrPtr->black, font, 0, 0, fPtr->caption, tlen);
      XCopyArea(display, d, view->window, scrPtr->copyGC, 0, 0, tw, th, tx, ty);
      XFreePixmap(display, d);
    } else {
      WMDrawString(scrPtr, view->window, scrPtr->black, font, tx, ty, fPtr->caption, tlen);
    }
  }
}

static void handleEvents(XEvent *event, void *data)
{
  WMFrame *fPtr = (WMFrame *) data;

  CHECK_CLASS(data, WC_Frame);

  switch (event->type) {
  case Expose:
    if (event->xexpose.count == 0)
      paintFrame(fPtr);
    break;

  case DestroyNotify:
    destroyFrame(fPtr);
    break;
  }
}

void WMSetFrameTitleColor(WMFrame *fPtr, WMColor *color)
{
  if (fPtr->textColor)
    WMReleaseColor(fPtr->textColor);
  fPtr->textColor = WMRetainColor(color);

  if (fPtr->view->flags.realized) {
    repaintFrame(fPtr);
  }
}

WMFrame *WMCreateFrame(WMWidget *parent)
{
  WMFrame *fPtr;

  fPtr = wmalloc(sizeof(WMFrame));

  fPtr->widgetClass = WC_Frame;

  fPtr->view = WMCreateView(WMWidgetView(parent));
  if (!fPtr->view) {
    wfree(fPtr);
    return NULL;
  }
  fPtr->view->self = fPtr;
  fPtr->textColor = WMRetainColor(fPtr->view->screen->black);

  WMCreateEventHandler(fPtr->view, ExposureMask | StructureNotifyMask, handleEvents, fPtr);

  fPtr->flags.relief = DEFAULT_RELIEF;
  fPtr->flags.titlePosition = DEFAULT_TITLE_POSITION;

  WMResizeWidget(fPtr, DEFAULT_WIDTH, DEFAULT_HEIGHT);

  return fPtr;
}

static void destroyFrame(WMFrame *fPtr)
{
  if (fPtr->caption)
    wfree(fPtr->caption);

  wfree(fPtr);
}
