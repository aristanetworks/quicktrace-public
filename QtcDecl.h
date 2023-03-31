// Copyright (c) 2017, Arista Networks, Inc.
// All rights reserved.

// Redistribution and use in source and binary forms, with or without modification,
// are permitted provided that the following conditions are met:

// 	* Redistributions of source code must retain the above copyright notice,
//  	  this list of conditions and the following disclaimer.
// 	* Redistributions in binary form must reproduce the above copyright notice,
// 	  this list of conditions and the following disclaimer in the documentation
// 	  and/or other materials provided with the distribution.
// 	* Neither the name of Arista Networks nor the names of its contributors may
// 	  be used to endorse or promote products derived from this software without
// 	  specific prior written permission.

// THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS "AS IS" AND
// ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE IMPLIED
// WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE ARE DISCLAIMED.
// IN NO EVENT SHALL ARISTA NETWORKS BE LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL,
// SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT LIMITED TO,
// PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES; LOSS OF USE, DATA, OR PROFITS; OR
// BUSINESS INTERRUPTION) HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN
// CONTRACT, STRICT LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN
// ANY WAY OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF
// SUCH DAMAGE.

#ifndef QUICKTRACE_QTCDECL_H
#define QUICKTRACE_QTCDECL_H

#include <stdbool.h>
#include <stdint.h>

extern void *default_hdl;

bool qt_initialize_default_handle( char const *filename, char const *sizeStr );
void *qt_initialize_handle( char const *filename, char const *sizeStr );
void qt_close_handle( void *hdl );
bool qt_isInitialized( void *hdl );
int  qt_msgDescSize();
void qt_msgDescInit( void *hdl, void *dp, int *msgidp, const char *f, int l );
void qt_addMsg( void *dp, const char *msg );
void qt_finish( void *dp );
void qt_startMsg( void *hdl, uint64_t *tsc, int id, int level );
void qt_endMsg( void *hdl, int level );
void qtprof_eob( void *tmp_var );
void qtproff_eob( void *tmp_var );

/*
 * The following put functions must have the type qt_put_<>_<>
 * so that they can be constructed by the qt_put_ macro.
 */
void qt_put_type_char( void *hdl, int n, void *d );
void qt_put_type_u8( void *hdl, int n, void *d );
void qt_put_type_u16( void *hdl, int n, void *d );
void qt_put_type_u32( void *hdl, int n, void *d );
void qt_put_type_u64( void *hdl, int n, void *d );
void qt_put_type_s8( void *hdl, int n, void *d );
void qt_put_type_s16( void *hdl, int n, void *d );
void qt_put_type_s32( void *hdl, int n, void *d );
void qt_put_type_s64( void *hdl, int n, void *d );
void qt_put_type_bool( void *hdl, int n, void *d );
void qt_put_type_double( void *hdl, int n, void *d );
void qt_put_type_float( void *hdl, int n, void *d );
void qt_put_type_int( void *hdl, int n, void *d );
void qt_put_type_charp( void *hdl, int n, void * d );
void qt_put_type_ptr( void *hdl, int n, void * d );
void qt_put_type_long( void *hdl, int n, void *d );
void qt_put_type_ulong( void *hdl, int n, void *d );

void qt_put_fmt_char( void *hdl, void *dp, void *d );
void qt_put_fmt_u8( void *hdl, void *dp, void *d );
void qt_put_fmt_u16( void *hdl, void *dp, void *d );
void qt_put_fmt_u32( void *hdl, void *dp, void *d );
void qt_put_fmt_u64( void *hdl, void *dp, void *d );
void qt_put_fmt_s8( void *hdl, void *dp, void *d );
void qt_put_fmt_s16( void *hdl, void *dp, void *d );
void qt_put_fmt_s32( void *hdl, void *dp, void *d );
void qt_put_fmt_s64( void *hdl, void *dp, void *d );
void qt_put_fmt_bool( void *hdl, void *dp, void *d );
void qt_put_fmt_double( void *hdl, void *dp, void *d );
void qt_put_fmt_float( void *hdl, void *dp, void *d );
void qt_put_fmt_int( void *hdl, void *dp, void *d );
void qt_put_fmt_charp( void *hdl, void *dp, void * d );
void qt_put_fmt_ptr( void *hdl, void *dp, void * d );
void qt_put_fmt_long( void *hdl, void *dp, void *d );
void qt_put_fmt_ulong( void *hdl, void *dp, void *d );

#endif // QUICKTRACE_QTCDECL_H
