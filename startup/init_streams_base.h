/**************************************************
 *
 * Common header for streams access to init data.
 *
 * Copyright 2015 IAR Systems. All rights reserved.
 *
 * $Revision: 52813 $
 *
 **************************************************/
#ifndef INIT_STREAMS_BASE_H
#define INIT_STREAMS_BASE_H

#include <limits.h>
#include <stdint.h>
#include <stdlib.h>

#pragma language=extended

// Define ptrdiff_t variants for each pointer type
#define __DATA_PTR_MEM_HELPER1__(M, I) \
typedef __DATA_MEM##I##_INDEX_TYPE__ M##_ptrdiff_t;
__DATA_PTR_MEMORY_LIST1__()
#undef __DATA_PTR_MEM_HELPER1__

#define TABLE_MEM _DLIB_ELF_INIT_TABLE_MEMORY
#ifdef _INITFMT
  #define SRC_MEM   _INITFMT_SOURCE_MEM
  #define DEST_MEM  _INITFMT_DEST_MEM
#else // ndef _INITFMT
  #define SRC_MEM   _DLIB_ELF_INIT_SOURCE_MEMORY
  #define DEST_MEM  _DLIB_ELF_INIT_DESTINATION_MEMORY
#endif // def _INITFMT

typedef _GLUE(SRC_MEM, _ptrdiff_t)  src_index_t;
typedef _GLUE(SRC_MEM, _size_t)     src_size_t;
typedef uint8_t SRC_MEM const *     src_ptr_t;

typedef _GLUE(DEST_MEM, _ptrdiff_t) dest_index_t;
typedef _GLUE(DEST_MEM, _size_t)    dest_size_t;
typedef uint8_t DEST_MEM      *     dest_ptr_t;

typedef _DLIB_ELF_INIT_REGION_COUNT_TYPE rgn_count_t;

#ifdef _DLIB_ELF_INIT_STATIC_BASE
static _DLIB_ELF_INIT_STATIC_BASE_DECLARATION;
#endif // _DLIB_ELF_INIT_STATIC_BASE

#ifndef _DLIB_ELF_INIT_SRC_RELATIVE_ROM_ADDRESSES
  #define _DLIB_ELF_INIT_SRC_RELATIVE_ROM_ADDRESSES \
                                     _DLIB_ELF_INIT_USE_RELATIVE_ROM_ADDRESSES
#endif // _DLIB_ELF_INIT_SRC_RELATIVE_ROM_ADDRESSES

#ifndef _DLIB_ELF_INIT_COMPUTE_SRC_ADDRESS
#define _DLIB_ELF_INIT_COMPUTE_SRC_ADDRESS(X,Y) ((src_ptr_t)X + Y)
#endif // _DLIB_ELF_INIT_COMPUTE_SRC_ADDRESS

// This is used for source address entries in the init table.
typedef struct
{
#if _DLIB_ELF_INIT_SRC_RELATIVE_ROM_ADDRESSES
  src_index_t mOff;
#else // !_DLIB_ELF_INIT_SRC_RELATIVE_ROM_ADDRESSES
  src_ptr_t mPtr;  
#endif // _DLIB_ELF_INIT_SRC_RELATIVE_ROM_ADDRESSES
} RAddr;

_DLIB_ELF_INIT_FUNCTION_ATTRIBUTES
static src_ptr_t RAddr_GetPtr(RAddr TABLE_MEM const * me)
{
#if _DLIB_ELF_INIT_SRC_RELATIVE_ROM_ADDRESSES
  return _DLIB_ELF_INIT_COMPUTE_SRC_ADDRESS(me, me->mOff);
#else // !_DLIB_ELF_INIT_SRC_RELATIVE_ROM_ADDRESSES
  return me->mPtr;
#endif // _DLIB_ELF_INIT_SRC_RELATIVE_ROM_ADDRESSES
}

// This is a "template", defining a struct with either one member of
// the given type, or two bitfield members of the given type, one of which
// is one bit wide and is a flag indicating use of a static base.
#ifdef _DLIB_ELF_INIT_STATIC_BASE
#define DEFINE_SB_COUNT_STRUCT(Name, Type)   \
  typedef struct                             \
  {                                          \
    Type mUseStaticBase : 1;                 \
    Type mVal : CHAR_BIT * sizeof(Type) - 1; \
  } Name;
#else // !_DLIB_ELF_INIT_STATIC_BASE
#define DEFINE_SB_COUNT_STRUCT(Name, Type)   \
  typedef struct                             \
  {                                          \
    Type mVal;                               \
  } Name;
#endif // _DLIB_ELF_INIT_STATIC_BASE

#endif /* INIT_STREAMS_BASE_HH */
