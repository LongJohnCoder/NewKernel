/** @file
Setup mouse driver.
Copyright (c) 2017, Byosoft Software Corporation. All Rights Reserved.

You may not reproduce, distribute, publish, display, perform, modify, adapt,
transmit, broadcast, present, recite, release, license or otherwise exploit
any part of this publication in any form, by any means, without the prior
written permission of Byosoft Software Corporation.

File Name:
  SetupMouseFuncs.c

Abstract:
  Setup Mouse Protocol implementation

Revision History:

**/

#include "SetupMouse.h"

BOOLEAN
IsRectEmpty (
  IN CONST RECT *Rc
  )
{
  return (BOOLEAN) (Rc->right == Rc->left || Rc->bottom == Rc->top);
}

VOID
SetRectEmpty (
  IN RECT *Rc
  )
{
  Rc->left = Rc->right = Rc->top = Rc->bottom = 0;
}

BOOLEAN
IntersectRect(
  OUT RECT *DstRc,
  IN CONST RECT *Src1,
  IN CONST RECT *Src2
  )
{
  if (IsRectEmpty(Src1) || IsRectEmpty(Src2) ||
      (Src1->left >= Src2->right) || (Src2->left >= Src1->right) ||
      (Src1->top >= Src2->bottom) || (Src2->top >= Src1->bottom)) {
    SetRectEmpty( DstRc );
    return FALSE;
  }
  
  DstRc->left   = MAX(Src1->left, Src2->left);
  DstRc->right  = MIN(Src1->right, Src2->right);
  DstRc->top    = MAX(Src1->top, Src2->top);
  DstRc->bottom = MIN(Src1->bottom, Src2->bottom);
  return TRUE;
}

BOOLEAN
OffsetRect(
  RECT *lprc,
  INT32 dx,
  INT32 dy
  )
{
  lprc->left += dx;
  lprc->right += dx;
  lprc->top += dy;
  lprc->bottom += dy;
  return TRUE;
}

VOID
SetRect(
  IN OUT RECT   *Rc,
  IN     INT32  Left,
  IN     INT32  Top,
  IN     INT32  Right,
  IN     INT32  Bottom
  )
{
  Rc->left   = Left;
  Rc->top    = Top;
  Rc->right  = Right;
  Rc->bottom = Bottom;
}

BOOLEAN
PtInRect (
  IN RECT  *Rc,
  IN POINT Pt
  )
{
  return (BOOLEAN) (Pt.x >= Rc->left && Pt.x < Rc->right &&
                    Pt.y >= Rc->top && Pt.y < Rc->bottom);
}

BOOLEAN
UnionRect(
  OUT      RECT *DstRc,
  IN CONST RECT *Src1,
  IN CONST RECT *Src2
  )
{
  if (IsRectEmpty(Src1)) {
    if (IsRectEmpty(Src2)) {
      SetRectEmpty( DstRc );
      return FALSE;
    } else {
      *DstRc = *Src2;
    }
  } else {
    if (IsRectEmpty(Src2)) {
      *DstRc = *Src1;
    } else {
      DstRc->left   = MIN( Src1->left, Src2->left );
      DstRc->right  = MAX( Src1->right, Src2->right );
      DstRc->top    = MIN( Src1->top, Src2->top );
      DstRc->bottom = MAX( Src1->bottom, Src2->bottom );
    }
  }
  return TRUE;
}

VOID
CollectChangedImage (
  IN IMAGE_INFO    *ImageInfo
  )
{
  GOP_ENTRY    *GopEntry;
  UINTN    Index;

  if (!ImageInfo->Visible) {
    return ;
  }

  for (Index = 0; Index < mPrivate->GopCount; Index++) {
    GopEntry = &mPrivate->GopList[Index];
    UnionRect (&GopEntry->InvalidateRc, &GopEntry->InvalidateRc, &ImageInfo->ImageRc);
  }
}

VOID
HideImage (
  IN  IMAGE_INFO    *ImageInfo
  )
{
  if (!ImageInfo->Visible) {
    return ;
  }

  CollectChangedImage (ImageInfo);
  ImageInfo->Visible = FALSE;
}

VOID
ShowImage (
  IN  IMAGE_INFO    *ImageInfo
  )
{
  if (ImageInfo->Visible) {
    return ;
  }

  ImageInfo->Visible = TRUE;
  CollectChangedImage (ImageInfo);
}

VOID
MoveImage (
  IN  IMAGE_INFO    *ImageInfo,
  IN  UINTN    X,
  IN  UINTN    Y
  )
{
  if (ImageInfo->Visible) {
    CollectChangedImage (ImageInfo);
  }
  
  OffsetRect (
    &ImageInfo->ImageRc,
    (INT32)(X - ImageInfo->ImageRc.left),
    (INT32)(Y - ImageInfo->ImageRc.top)
    );

  if (ImageInfo->Visible) {
    CollectChangedImage (ImageInfo);
  }
}

VOID
DestroyImage (
  IN IMAGE_INFO    *ImageInfo
  )
{
  if (ImageInfo->Image != NULL) {
    gBS->FreePool (ImageInfo->Image);
    ImageInfo->Image = NULL;
  }
}
