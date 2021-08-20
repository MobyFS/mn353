/**************************************************
 *
 * Implements stream access to the init data for
 * single range decompression.
 *
 * Copyright 2015 IAR Systems. All rights reserved.
 *
 * $Revision: 52625 $
 *
 **************************************************/
#ifndef INIT_STREAMS_H
#define INIT_STREAMS_H

#include "init_streams_base.h"

/* Format:
  SrcAddr
  SrcSize
  DestAddr
*/

// The pragma is there to make sure that the mUseStaticBase 
// part of the structure always is the lsb.
#pragma bitfields=disjoint_types
DEFINE_SB_COUNT_STRUCT(SourceSize, src_size_t)
#pragma bitfields=default

typedef struct
{
  RAddr      mSrc;
  SourceSize mSize;
  dest_ptr_t mDest;
} TableEntryHead;

typedef TableEntryHead TABLE_MEM const * thead_ptr_t;

typedef struct
{
  src_ptr_t mCurrentPos;
  src_ptr_t mRangeEnd;
} InStream;

_DLIB_ELF_INIT_FUNCTION_ATTRIBUTES
static void InStream_init(InStream * me, thead_ptr_t p)
{
  me->mCurrentPos = RAddr_GetPtr(&p->mSrc);
  me->mRangeEnd   = me->mCurrentPos + p->mSize.mVal;
}

_DLIB_ELF_INIT_FUNCTION_ATTRIBUTES
static int InStream_AtEnd(InStream const * me)
{
  return me->mCurrentPos == me->mRangeEnd;
}

_DLIB_ELF_INIT_FUNCTION_ATTRIBUTES
static uint8_t InStream_Peek(InStream * me)
{
  return *(me->mCurrentPos);
}

_DLIB_ELF_INIT_FUNCTION_ATTRIBUTES
static uint8_t InStream_Read(InStream * me)
{
  return *((me->mCurrentPos)++);
}

typedef struct
{
  dest_ptr_t  mCurrentPos;
  thead_ptr_t mAfter;
} OutStream;

_DLIB_ELF_INIT_FUNCTION_ATTRIBUTES
static void OutStream_init(OutStream * me, thead_ptr_t p)
{
  me->mCurrentPos = p->mDest;
#ifdef _DLIB_ELF_INIT_STATIC_BASE
  if (p->mSize.mUseStaticBase)
    me->mCurrentPos += _DLIB_ELF_INIT_STATIC_BASE;
#endif // _DLIB_ELF_INIT_STATIC_BASE
  me->mAfter = p + 1;
}

_DLIB_ELF_INIT_FUNCTION_ATTRIBUTES
static void OutStream_Write(OutStream * me, uint8_t c)
{
  *((me->mCurrentPos)++) = c;
}

_DLIB_ELF_INIT_FUNCTION_ATTRIBUTES
static void TABLE_MEM const * OutStream_End(OutStream const * me)
{
  return me->mAfter;
}

/* For reading from an out stream */
typedef struct
{
  uint8_t DEST_MEM const * mCurrentPos;
} OInStream;

// Return an in stream pointing at an earlier place in the out stream
_DLIB_ELF_INIT_FUNCTION_ATTRIBUTES
static OInStream OutStream_GetOInStream(OutStream const * me, uint32_t backOff)
{
  OInStream in;
  in.mCurrentPos = me->mCurrentPos - backOff;
  return in;
}

_DLIB_ELF_INIT_FUNCTION_ATTRIBUTES
static uint8_t OInStream_Read(OInStream * me)
{
  return *((me->mCurrentPos)++);
}

#endif /* INIT_STREAMS_H */
