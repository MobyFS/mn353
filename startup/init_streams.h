/**************************************************
 *
 * Implements stream access to the init data.
 *
 * Copyright 2008 IAR Systems. All rights reserved.
 *
 * $Revision: 52789 $
 *
 **************************************************/
#ifndef INIT_STREAMS_H
#define INIT_STREAMS_H

#include "init_streams_base.h"

#include <limits.h>
#include <stdint.h>
#include <stdlib.h>

#pragma language=extended

/* Format:
  NumberOfSources (N)
  NumberOfDestinations (M)
  SrcAddr1
  SrcSize1
  ...
  SrcAddrN
  SrcSizeN
  DestAddr1
  DestSize1
  ...
  DestAddrM
  DestSizeM
*/

typedef struct
{
  RAddr mAddr;
  src_size_t mSize;
} RElem;

// The pragma is there to make sure that the mUseStaticBase 
// part of the structure always is the lsb.
#pragma bitfields=disjoint_types
DEFINE_SB_COUNT_STRUCT(RegionCount, rgn_count_t)
#pragma bitfields=default

typedef struct
{
  RegionCount mSourceCount;
  RegionCount mDestinationCount;
  RElem mSources[];
} TableEntryHead;

typedef TableEntryHead TABLE_MEM const * thead_ptr_t;

typedef struct
{
  RElem TABLE_MEM const * mRangeStart;
  RElem TABLE_MEM const * mDataEnd;
  src_ptr_t               mCurrentPos;
  src_ptr_t               mRangeEnd;
} InStream;

_DLIB_ELF_INIT_FUNCTION_ATTRIBUTES
static void InStream_init(InStream * me, thead_ptr_t p)
{
  me->mRangeStart = p->mSources;
  me->mDataEnd = me->mRangeStart + p->mSourceCount.mVal;
  me->mCurrentPos = 0;
  me->mRangeEnd = 0;
}

_DLIB_ELF_INIT_FUNCTION_ATTRIBUTES
static void InStream_StepRegion(InStream * me)
{
  me->mCurrentPos = RAddr_GetPtr(&me->mRangeStart->mAddr);
  me->mRangeEnd   = me->mCurrentPos + me->mRangeStart->mSize;
  ++me->mRangeStart;
}

_DLIB_ELF_INIT_FUNCTION_ATTRIBUTES
static int InStream_AtEnd(InStream * me)
{
  while (me->mCurrentPos == me->mRangeEnd)
  {
    if (me->mRangeStart == me->mDataEnd)
      return 1;
    InStream_StepRegion(me);
  }
  return 0;
}

_DLIB_ELF_INIT_FUNCTION_ATTRIBUTES
static void InStream_MaybeStepRegion(InStream * me)
{
  /* Check if we reached the end of a source region */
  if (me->mCurrentPos == me->mRangeEnd)
    InStream_StepRegion(me);
}

_DLIB_ELF_INIT_FUNCTION_ATTRIBUTES
static uint8_t InStream_Peek(InStream * me)
{
  // TODO: Do not change region here!
  InStream_MaybeStepRegion(me);
  return *(me->mCurrentPos);
}

_DLIB_ELF_INIT_FUNCTION_ATTRIBUTES
static uint8_t InStream_Read(InStream * me)
{
  InStream_MaybeStepRegion(me);
  return *((me->mCurrentPos)++);
}

typedef struct
{
  dest_ptr_t mPtr;
  dest_size_t mSize;
} WElem;

typedef struct
{
  WElem TABLE_MEM const * mRangeStart;
  WElem TABLE_MEM const * mDataEnd;
  dest_ptr_t              mCurrentPos;
  dest_ptr_t              mRangeEnd;
#ifdef _DLIB_ELF_INIT_STATIC_BASE
  _Bool                   mUseStaticBase;
#endif // _DLIB_ELF_INIT_STATIC_BASE
} OutStream;

_DLIB_ELF_INIT_FUNCTION_ATTRIBUTES
static void OutStream_init(OutStream * me, thead_ptr_t p)
{
  me->mRangeStart = (WElem TABLE_MEM const *)&p->mSources[p->mSourceCount.mVal];
#ifdef _DLIB_ELF_INIT_STATIC_BASE
  me->mUseStaticBase = p->mDestinationCount.mUseStaticBase;
#endif // _DLIB_ELF_INIT_STATIC_BASE
  me->mDataEnd = me->mRangeStart + p->mDestinationCount.mVal;
  me->mCurrentPos = 0;
  me->mRangeEnd = 0;
}

_DLIB_ELF_INIT_FUNCTION_ATTRIBUTES
static int OutStream_AtEnd(OutStream const * me)
{
  return me->mCurrentPos == me->mRangeEnd && me->mRangeStart == me->mDataEnd;
}

_DLIB_ELF_INIT_FUNCTION_ATTRIBUTES
static void OutStream_StepRegion(OutStream * me)
{
  while (me->mCurrentPos == me->mRangeEnd)
  {
    if (me->mRangeStart == me->mDataEnd)
      abort();

    me->mCurrentPos = me->mRangeStart->mPtr;

#ifdef _DLIB_ELF_INIT_STATIC_BASE
    if (me->mUseStaticBase)
      me->mCurrentPos += _DLIB_ELF_INIT_STATIC_BASE;
#endif // _DLIB_ELF_INIT_STATIC_BASE

    me->mRangeEnd = me->mCurrentPos + me->mRangeStart->mSize;

    ++me->mRangeStart;
  }
}

_DLIB_ELF_INIT_FUNCTION_ATTRIBUTES
static void OutStream_Write(OutStream * me, uint8_t c)
{
  if (me->mCurrentPos == me->mRangeEnd) /* End of destination buffers */
    OutStream_StepRegion(me);

  *((me->mCurrentPos)++) = c;
}

_DLIB_ELF_INIT_FUNCTION_ATTRIBUTES
static void TABLE_MEM const * OutStream_End(OutStream const * me)
{
  return me->mDataEnd;
}

/* For reading from an out stream */
typedef struct
{
  WElem TABLE_MEM const *  mRangeStart;
  WElem TABLE_MEM const *  mDataEnd;
  uint8_t DEST_MEM const * mCurrentPos;
  uint8_t DEST_MEM const * mRangeEnd;
} OInStream;

/* Back up backOff bytes. */
_DLIB_ELF_INIT_FUNCTION_ATTRIBUTES
static void OInStream_Backup(OInStream * me, uint32_t backOff)
{
  for (;;)
  {
    uint32_t here = me->mCurrentPos - me->mRangeStart[-1].mPtr;
    if (backOff <= here)
    {
      me->mCurrentPos -= backOff;
      break;
    }
    backOff -= here;
    --me->mRangeStart;
    me->mRangeEnd = me->mRangeStart[-1].mPtr + me->mRangeStart[-1].mSize;
    me->mCurrentPos = me->mRangeEnd;
  }
}

// Return an in stream pointing at an earlier place in the out stream
_DLIB_ELF_INIT_FUNCTION_ATTRIBUTES
static OInStream OutStream_GetOInStream(OutStream const * me, uint32_t backOff)
{
  OInStream in;
  in.mRangeStart = me->mRangeStart;
  in.mDataEnd    = me->mDataEnd;
  in.mCurrentPos = me->mCurrentPos;
  in.mRangeEnd   = me->mRangeEnd;
  OInStream_Backup(&in, backOff);
  return in;
}

_DLIB_ELF_INIT_FUNCTION_ATTRIBUTES
static void OInStream_StepRegion(OInStream * me)
{
  /* Check if we reached the end of a source region */
  while (me->mCurrentPos == me->mRangeEnd)
  {
    me->mCurrentPos = me->mRangeStart->mPtr;
    me->mRangeEnd   = me->mCurrentPos + me->mRangeStart->mSize;
    ++me->mRangeStart;
  }
}

_DLIB_ELF_INIT_FUNCTION_ATTRIBUTES
static uint8_t OInStream_Read(OInStream * me)
{
  if (me->mCurrentPos == me->mRangeEnd)
    OInStream_StepRegion(me);
  return *((me->mCurrentPos)++);
}

#endif /* INIT_STREAMS_H */
