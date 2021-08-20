/**************************************************
 *
 * Configurable generic initialization routines for Ilink products.
 *
 * Copyright 2012 IAR Systems. All rights reserved.
 *
 * $Revision: 52804 $
 *
 **************************************************/

#include <stdint.h>

/*** CONFIGURATION VARIABLES ***/

// _DLIB_ELF_INIT_TABLE_MEMORY is the memory used for pointers into the
// init table itself. That is, a pointer into the init table is a
// "void _DLIB_ELF_INIT_TABLE_MEMORY const *".
#ifndef _DLIB_ELF_INIT_TABLE_MEMORY
  #error "Init table memory not set"
#endif // _DLIB_ELF_INIT_TABLE_MEMORY

// _DLIB_ELF_INIT_SOURCE_MEMORY is the memory used for pointers to
// initializer bytes.
#ifndef _DLIB_ELF_INIT_SOURCE_MEMORY
  #error "Init source memory not set"
#endif // _DLIB_ELF_INIT_SOURCE_MEMORY

// _DLIB_ELF_INIT_DESTINATION_MEMORY is the memory used for pointers to
// the initialized variables.
#ifndef _DLIB_ELF_INIT_DESTINATION_MEMORY
  #error "Init destination memory not set"
#endif // _DLIB_ELF_INIT_DESTINATION_MEMORY

// _DLIB_ELF_INIT_REGION_COUNT_TYPE is an integer type used for the number
// of source or destination regions. In typical cases it is used only a few
// times in the init table, so its effect on memory usage is very limited.
#ifndef _DLIB_ELF_INIT_REGION_COUNT_TYPE
  #define _DLIB_ELF_INIT_REGION_COUNT_TYPE unsigned int
#endif // _DLIB_ELF_INIT_REGION_COUNT_TYPE

// _DLIB_ELF_INIT_USE_RELATIVE_ROM_ADDRESSES is 1 or 0. If it is 0, function and
// source address entries in the init table hold absolute addresses,
// otherwise they hold offsets from themselves. If not defined, absolute
// addresses are used.
// Relative ROM addresses work both when using ROPI and when not using ROPI at
// the cost of a very small overhead in code size.
// In order for relative addresses to work, code and constants must be in the
// same address space.
// If necessary, _DLIB_ELF_INIT_SRC_RELATIVE_ROM_ADDRESSES can be used for 
// source data addresses if their handling is different from functions.

#ifndef _DLIB_ELF_INIT_USE_RELATIVE_ROM_ADDRESSES
  #define _DLIB_ELF_INIT_USE_RELATIVE_ROM_ADDRESSES 0
#endif // _DLIB_ELF_INIT_USE_RELATIVE_ROM_ADDRESSES

#ifndef _DLIB_ELF_INIT_SRC_RELATIVE_ROM_ADDRESSES
  #define _DLIB_ELF_INIT_SRC_RELATIVE_ROM_ADDRESSES \
                                     _DLIB_ELF_INIT_USE_RELATIVE_ROM_ADDRESSES
#endif // _DLIB_ELF_INIT_SRC_RELATIVE_ROM_ADDRESSES

// _DLIB_ELF_INIT_STATIC_BASE_DECLARATION and _DLIB_ELF_INIT_STATIC_BASE
// are used to turn on support for adding a static base address to destination
// entries in the init table. You must either define both, or neither. If they
// are defined, the code is designed to work whether RWPI is in use or not.
// If static base support is enabled, a bit in the init table encodes whether it
// is actually used or not.
// _DLIB_ELF_INIT_STATIC_BASE_DECLARATION is a declaration of the static base
// variable to use. Something like this:
//     __no_init uint32_t __iar_SB @ r9
// NOTE: No semicolon at the end, and no storage class.
// _DLIB_ELF_INIT_STATIC_BASE is the name of the static base variable.
#if    defined(_DLIB_ELF_INIT_STATIC_BASE) \
    != defined(_DLIB_ELF_INIT_STATIC_BASE_DECLARATION)
  #error "Either both of _DLIB_ELF_INIT_STATIC_BASE_DECLARATION and " \
         "_DLIB_ELF_INIT_STATIC_BASE must be defined, or none of them"
#endif // _DLIB_ELF_INIT_STATIC_BASE

// _DLIB_ELF_INIT_MODULE_ATTRIBUTES is used near the top of each init routine
// source file. It is expected that it will contain a series of _Pragma()
// operators to set up runtime model attributes or similar things, if that is
// needed. If you don't need module attributes, you don't need to define it.

#ifndef _DLIB_ELF_INIT_MODULE_ATTRIBUTES
  #define _DLIB_ELF_INIT_MODULE_ATTRIBUTES
#endif // _DLIB_ELF_INIT_MODULE_ATTRIBUTES

// _DLIB_ELF_INIT_FUNCTION_ATTRIBUTES is used to give all init routines and
// helper routines a set of function attributes. The ARM product uses this to
// always generate thumb code for these routines, as they are typically only
// executed once, at program startup. If you do not need any non-default
// function attributes, you do not need to define this.
#ifndef _DLIB_ELF_INIT_FUNCTION_ATTRIBUTES
  #define _DLIB_ELF_INIT_FUNCTION_ATTRIBUTES
#endif // _DLIB_ELF_INIT_FUNCTION_ATTRIBUTES

// _DLIB_ELF_INIT_INTERFACE_VERSION is used as a suffix to the names of all
// init routines. It corresponds to the interface version used in
// LpCompression::TargetControl::GetInitRoutineName.
// Interface version 1, which is the default, results in no suffix.
#if _DLIB_ELF_INIT_INTERFACE_VERSION == 1
  #undef _DLIB_ELF_INIT_INTERFACE_VERSION
#endif // _DLIB_ELF_INIT_INTERFACE_VERSION
#ifndef _DLIB_ELF_INIT_INTERFACE_VERSION
  #define _DLIB_ELF_INIT_INTERFACE_VERSION
#endif // _DLIB_ELF_INIT_INTERFACE_VERSION

#pragma language = extended

// The type of a pointer into the init table.
typedef void _DLIB_ELF_INIT_TABLE_MEMORY const * table_ptr_t;

// This is for compiling each init routine in several variants.
// To do so, define _INITFMT to be the extra part of the name of the routines
// ("_near" in __iar_zero_init_near2", for instance).
// Also define _INITFMT_SOURCE_MEM and _INITFMT_DEST_MEM to be the source
// and destination memory, respectively, for the init routine variant.

#ifdef _INITFMT
  #ifndef _INITFMT_SOURCE_MEM
    #error "Init format source memory not set"
  #endif // ndef _INITFMT_SOURCE_MEM
  #ifndef _INITFMT_DEST_MEM
    #error "Init format destination memory not set"
  #endif // ndef _INITFMT_DEST_MEM
  #define _NAMEFMT0 _INITFMT
#else // ndef _INITFMT
  #define _NAMEFMT0
#endif // def _INITFMT

#ifdef _SUFFIX
  #define _NAMEFMT _GLUE(_NAMEFMT0,_SUFFIX)
#else // ndef _SUFFIX
  #define _NAMEFMT _NAMEFMT0
#endif // def _SUFFIX

// The type of an initialization routine. It takes a pointer to the start of
// its entry (after the function pointer) in the init table and returns a
// pointer to after its entry.
_DLIB_ELF_INIT_FUNCTION_ATTRIBUTES
typedef table_ptr_t init_fun_t(table_ptr_t);

#define IAR_DATA_INIT _GLUE(__iar_data_init, _DLIB_ELF_INIT_INTERFACE_VERSION)
#define IAR_COPY_INIT(fmt)     \
             _GLUE(_GLUE(__iar_copy_init, fmt), _DLIB_ELF_INIT_INTERFACE_VERSION)
#define IAR_ZERO_INIT(fmt)     \
             _GLUE(_GLUE(__iar_zero_init, fmt), _DLIB_ELF_INIT_INTERFACE_VERSION)
#define IAR_RLE_INIT(fmt)      \
              _GLUE(_GLUE(__iar_rle_init, fmt), _DLIB_ELF_INIT_INTERFACE_VERSION)
#define IAR_PACKBITS_INIT(fmt) \
         _GLUE(_GLUE(__iar_packbits_init, fmt), _DLIB_ELF_INIT_INTERFACE_VERSION)
#define IAR_LZW_INIT(fmt)      \
              _GLUE(_GLUE(__iar_lzw_init, fmt), _DLIB_ELF_INIT_INTERFACE_VERSION)
#define IAR_BWT_INIT(fmt)      \
              _GLUE(_GLUE(__iar_bwt_init, fmt), _DLIB_ELF_INIT_INTERFACE_VERSION)
#define IAR_LZ77_INIT(fmt)     \
             _GLUE(_GLUE(__iar_lz77_init, fmt), _DLIB_ELF_INIT_INTERFACE_VERSION)

_DLIB_ELF_INIT_FUNCTION_ATTRIBUTES
void IAR_DATA_INIT(void);
init_fun_t IAR_COPY_INIT(_NAMEFMT);
init_fun_t IAR_ZERO_INIT(_NAMEFMT);
init_fun_t IAR_RLE_INIT(_NAMEFMT);
init_fun_t IAR_PACKBITS_INIT(_NAMEFMT);
init_fun_t IAR_LZW_INIT(_NAMEFMT);
init_fun_t IAR_BWT_INIT(_NAMEFMT);
init_fun_t IAR_LZ77_INIT(_NAMEFMT);
