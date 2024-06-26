/*
 *  Workspace window manager
 *  Copyright (c) 2015-2021 Sergii Stoian
 *
 *  WINGs library (Window Maker)
 *  Copyright (c) 1998 scottc
 *  Copyright (c) 1999-2004 Dan Pascu
 *  Copyright (c) 1999-2000 Alfredo K. Kojima
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

#include "WM.h"

#include <core/WMcore.h>
#include <core/util.h>

#include "log_utils.h"
#include "wscreen.h"
#include "dragcommon.h"
#include "dragdestination.h"
#include "wpixmap.h"
#include "drawing.h"

#define XDND_SOURCE_VERSION(dragInfo) dragInfo->protocolVersion
#define XDND_DEST_INFO(dragInfo) dragInfo->destInfo
#define XDND_DEST_VIEW(dragInfo) dragInfo->destInfo->destView

static Bool _WindowExists;

Atom WMOperationToAction(WMScreen *scr, WMDragOperationType operation)
{
  switch (operation) {
  case WDOperationNone:
    return None;

  case WDOperationCopy:
    return scr->xdndActionCopy;

  case WDOperationMove:
    return scr->xdndActionMove;

  case WDOperationLink:
    return scr->xdndActionLink;

  case WDOperationAsk:
    return scr->xdndActionAsk;

  case WDOperationPrivate:
    return scr->xdndActionPrivate;

  default:
    return None;
  }
}

WMDragOperationType WMActionToOperation(WMScreen *scr, Atom action)
{
  if (action == scr->xdndActionCopy) {
    return WDOperationCopy;

  } else if (action == scr->xdndActionMove) {
    return WDOperationMove;

  } else if (action == scr->xdndActionLink) {
    return WDOperationLink;

  } else if (action == scr->xdndActionAsk) {
    return WDOperationAsk;

  } else if (action == scr->xdndActionPrivate) {
    return WDOperationPrivate;

  } else if (action == None) {

    return WDOperationNone;
  } else {
    char *tmp = XGetAtomName(scr->display, action);

    WMLogWarning(_("unknown XDND action %s"), tmp);
    XFree(tmp);

    return WDOperationCopy;
  }
}

static void freeDragOperationItem(CFAllocatorRef allocator, const void *item)
{
  wfree((void *)item);
}

CFMutableArrayRef WMCreateDragOperationArray(int initialSize)
{
  CFArrayCallBacks cbs = {0, NULL, freeDragOperationItem, NULL, NULL};
  return CFArrayCreateMutable(kCFAllocatorDefault, initialSize, &cbs);
}

WMDragOperationItem *WMCreateDragOperationItem(WMDragOperationType type, char *text)
{
  WMDragOperationItem *result = wmalloc(sizeof(WMDragOperationItem));

  result->type = type;
  result->text = text;

  return (WMDragOperationItem *) result;
}

WMDragOperationType WMGetDragOperationItemType(WMDragOperationItem *item)
{
  return ((WMDragOperationItem *) item)->type;
}

char *WMGetDragOperationItemText(WMDragOperationItem *item)
{
  return ((WMDragOperationItem *) item)->text;
}

static int handleNoWindowXError(Display *dpy, XErrorEvent *errEvt)
{
  /* Parameter not used, but tell the compiler that it is ok */
  (void) dpy;

  if (errEvt->error_code == BadWindow || errEvt->error_code == BadDrawable) {
    _WindowExists = False;
    return Success;
  }

  return errEvt->error_code;
}

static Bool windowExists(Display *dpy, Window win)
{
  void *previousErrorHandler;
  XWindowAttributes attr;

  XSynchronize(dpy, True);
  previousErrorHandler = XSetErrorHandler(handleNoWindowXError);
  _WindowExists = True;

  /* can generate BadDrawable or BadWindow */
  XGetWindowAttributes(dpy, win, &attr);

  XSetErrorHandler(previousErrorHandler);
  XSynchronize(dpy, False);
  return _WindowExists;
}

Bool
WMSendDnDClientMessage(Display *dpy, Window win, Atom message,
		       unsigned long data0,
		       unsigned long data1, unsigned long data2, unsigned long data3, unsigned long data4)
{
  XEvent ev;

#ifdef XDND_DEBUG
  char *msgName = XGetAtomName(dpy, message);

  printf("sending message %s ... ", msgName);
  XFree(msgName);
#endif

  if (!windowExists(dpy, win)) {
    WMLogWarning(_("target %lu for XDND message no longer exists"), win);
    return False;	/* message not sent */
  }

  ev.type = ClientMessage;
  ev.xclient.message_type = message;
  ev.xclient.format = 32;
  ev.xclient.window = win;
  ev.xclient.data.l[0] = data0;
  ev.xclient.data.l[1] = data1;
  ev.xclient.data.l[2] = data2;
  ev.xclient.data.l[3] = data3;
  ev.xclient.data.l[4] = data4;

  XSendEvent(dpy, win, False, 0, &ev);
  XFlush(dpy);

#ifdef XDND_DEBUG
  printf("sent\n");
#endif
  return True;		/* message sent */
}

static void handleLeaveMessage(WMDraggingInfo *info)
{
  if (XDND_DEST_INFO(info) != NULL) {
    /* XDND_DEST_VIEW is never NULL (it's the xdnd aware view) */
    wassertr(XDND_DEST_VIEW(info) != NULL);
    if (XDND_DEST_VIEW(info)->dragDestinationProcs != NULL) {
      XDND_DEST_VIEW(info)->dragDestinationProcs->concludeDragOperation(XDND_DEST_VIEW(info));
    }
    WMDragDestinationInfoClear(info);
  }
}

void WMHandleDNDClientMessage(WMView *toplevel, XClientMessageEvent *event)
{
  WMScreen *scr = WMViewScreen(toplevel);
  WMDraggingInfo *info = scr->dragInfo;
  Atom messageType = event->message_type;

#ifdef XDND_DEBUG
  {
    char *msgTypeName = XGetAtomName(scr->display, messageType);

    if (msgTypeName != NULL)
      printf("event type = %s\n", msgTypeName);
    else
      printf("pb with event type !\n");
  }
#endif

  /* Messages from destination to source */
  if (messageType == scr->xdndStatusAtom || messageType == scr->xdndFinishedAtom) {
    WMDragSourceStopTimer();
    WMDragSourceStateHandler(info, event);
    return;
  }

  /* Messages from source to destination */
  if (messageType == scr->xdndEnterAtom) {
    Bool positionSent = (XDND_DEST_INFO(info) != NULL);

    WMDragDestinationStopTimer();
    WMDragDestinationStoreEnterMsgInfo(info, toplevel, event);

    /* Xdnd version 3 and up are not compatible with version 1 or 2 */
    if (XDND_SOURCE_VERSION(info) > 2) {

      if (positionSent) {
        /* xdndPosition previously received on xdnd aware view */
        WMDragDestinationStateHandler(info, event);
        return;
      } else {
        WMDragDestinationStartTimer(info);
        return;
      }
    } else {
      WMLogWarning(_("unsupported version %i for XDND enter message"), XDND_SOURCE_VERSION(info));
      WMDragDestinationCancelDropOnEnter(toplevel, info);
      return;
    }
  }

  if (messageType == scr->xdndPositionAtom) {
    WMDragDestinationStopTimer();
    WMDragDestinationStorePositionMsgInfo(info, toplevel, event);
    WMDragDestinationStateHandler(info, event);
    return;
  }

  if (messageType == scr->xdndSelectionAtom || messageType == scr->xdndDropAtom) {
    WMDragDestinationStopTimer();
    WMDragDestinationStateHandler(info, event);
    return;
  }

  if (messageType == scr->xdndLeaveAtom) {
    /* conclude drop operation, and clear dragging info */
    WMDragDestinationStopTimer();
    handleLeaveMessage(info);
  }
}

/* called in destroyView (wview.c) */
void WMFreeViewXdndPart(WMView *view)
{
  WMUnregisterViewDraggedTypes(view);

  if (view->dragSourceProcs)
    wfree(view->dragSourceProcs);

  if (view->dragDestinationProcs)
    wfree(view->dragDestinationProcs);

  if (view->dragImage)
    WMReleasePixmap(view->dragImage);
}
