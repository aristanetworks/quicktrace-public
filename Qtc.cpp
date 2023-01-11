// Copyright (c) 2011, Arista Networks, Inc.
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

#include <iostream>
#include <QuickTrace/QuickTrace.h>
#include <stdint.h>
#include <stdlib.h>

extern "C" {

#include <QuickTrace/QtcDecl.h>

void *default_hdl = 0;

void*
qt_initialize_handle ( char const *filename, char const *sizeStr )
{
   int i=0, ss[10] = {8,8,8,8,8,8,8,8,8,8};
   if( sizeStr ) {
      char * s2 = strdup( sizeStr );
      char * saveptr = NULL;
      char * c = strtok_r( s2, ", ", &saveptr );
      while( c ) {
         ss[i] = atoi( c ) ?: 1;
         c = strtok_r( NULL, ", ", &saveptr );
         i++;
      }
      free( s2 );
   }
   return QuickTrace::initialize_handle(filename, (QuickTrace::SizeSpec*)ss);
}

bool
qt_initialize_default_handle ( char const * filename, char const * sizeStr ) 
{
   if (default_hdl) {
      return true;
   }

   default_hdl = qt_initialize_handle(filename, sizeStr);
   if (default_hdl) {
      return true;
   } else {
      return false;
   }
}

void
qt_close_handle( void *hdl )
{
   QuickTrace::TraceHandle *th = (QuickTrace::TraceHandle *)hdl;
   if (th)
      th->close();
}

bool
qt_isInitialized (void *hdl)
{
   QuickTrace::TraceHandle *th = (QuickTrace::TraceHandle *)hdl;
   return (th && th->isInitialized());
}

int
qt_msgDescSize ()
{
    return(sizeof(QuickTrace::MsgDesc));
}

void
qt_msgDescInit ( void *hdl, void *dp, int *msgidp, const char *f, int l )
{
   QuickTrace::MsgDesc *desc = ( QuickTrace::MsgDesc * )dp;
   QuickTrace::TraceHandle *th = (QuickTrace::TraceHandle *)hdl;
   *desc = QuickTrace::MsgDesc( th->getFile(), (QuickTrace::MsgId *)msgidp, f, l );
}

void
qt_addMsg ( void *dp, const char *msg )
{
    QuickTrace::MsgDesc *desc = ( QuickTrace::MsgDesc * )dp;
    *desc << msg;
}

void
qt_finish ( void *dp )
{
    QuickTrace::MsgDesc *desc = ( QuickTrace::MsgDesc * )dp;
    desc->finish();
}

void
qt_startMsg ( void *hdl, uint64_t *tsc, int id, int level )
{
   QuickTrace::TraceHandle *th = (QuickTrace::TraceHandle *)hdl;
   QuickTrace::RingBuf & sl = (th->getFile())->log(level);     
   *tsc = sl.startMsg( th->getFile(), id );
}

void
qt_endMsg ( void *hdl, int level )
{
   QuickTrace::TraceHandle *th = (QuickTrace::TraceHandle *)hdl;
   QuickTrace::RingBuf & sl = (th->getFile())->log(level);
   sl.endMsg();
}

void
qtprof_eob ( void *tmp_var)
{
   qtprof_t *prof = (qtprof_t *)tmp_var;
   void *hdl = prof->th;
   if( likely( !!( qt_isInitialized(hdl) ) ) ) {
      uint64_t now = rdtsc();
      uint64_t delta = now - prof->tsc;
      QuickTrace::TraceHandle *thdl = (QuickTrace::TraceHandle *)hdl;
      QuickTrace::MsgCounter * mc = 
                   (thdl->getFile())->msgCounter( prof->mid );
      mc->tscCount += delta;
      mc->count++;
   }   
}

void
qtproff_eob ( void *tmp_var)
{
   qtprof_t *prof = (qtprof_t *)tmp_var;
   void *hdl = prof->th;
   if( likely( !!( qt_isInitialized(hdl) ) ) ) {
      uint64_t now = rdtsc();
      uint64_t delta = now - prof->tsc;
      QuickTrace::TraceHandle *thdl = (QuickTrace::TraceHandle *)hdl;
      QuickTrace::MsgCounter * mc = 
                   (thdl->getFile())->msgCounter( prof->mid );
      mc->tscCount += delta;
   }   
}

void qt_put_type_ptr ( void *hdl, int n, void * d )
{
    QuickTrace::TraceHandle *th = (QuickTrace::TraceHandle *)hdl; 
    QuickTrace::RingBuf & sl = ((th)->getFile())->log(n);
#if __SIZEOF_POINTER__ == 8
    sl << (void *)*((uint64_t*)d);
#else
    sl << (void *)*((uint32_t*)d);
#endif
}

void qt_put_fmt_ptr ( void *hdl, void *dp, void * d )
{
    QuickTrace::MsgDesc *desc = ( QuickTrace::MsgDesc * )dp; 
#if __SIZEOF_POINTER__ == 8
    desc->formatString() << (void *)*((uint64_t*)d);
#else
    desc->formatString() << (void *)*((uint32_t*)d);
#endif
}

#define QT_PUT_TYPE(_T)                                                  \
    QuickTrace::TraceHandle *th = (QuickTrace::TraceHandle *)hdl;        \
    QuickTrace::RingBuf & sl = ((th)->getFile())->log(n);                \
    sl << *((const _T*) d);         

#define QT_PUT_FMT(_T)                                                   \
    QuickTrace::MsgDesc *desc = ( QuickTrace::MsgDesc * )dp;             \
    desc->formatString() << *((const _T*)d);

void qt_put_type_char( void *hdl, int n, void *d )       {QT_PUT_TYPE(char)}
void qt_put_type_u8( void *hdl, int n, void *d )         {QT_PUT_TYPE(uint8_t)}
void qt_put_type_u16( void *hdl, int n, void *d )        {QT_PUT_TYPE(uint16_t)}
void qt_put_type_u32( void *hdl, int n, void *d )        {QT_PUT_TYPE(uint32_t)}
void qt_put_type_u64( void *hdl, int n, void *d )        {QT_PUT_TYPE(uint64_t)}
void qt_put_type_s8( void *hdl, int n, void *d )         {QT_PUT_TYPE(int8_t)}
void qt_put_type_s16( void *hdl, int n, void *d )        {QT_PUT_TYPE(int16_t)}
void qt_put_type_s32( void *hdl, int n, void *d )        {QT_PUT_TYPE(int32_t)}
void qt_put_type_s64( void *hdl, int n, void *d )        {QT_PUT_TYPE(int64_t)}
void qt_put_type_bool( void *hdl, int n, void *d )       {QT_PUT_TYPE(bool)}
void qt_put_type_double( void *hdl, int n, void *d )     {QT_PUT_TYPE(double)}
void qt_put_type_float( void *hdl, int n, void *d )      {QT_PUT_TYPE(float)}
void qt_put_type_int( void *hdl, int n, void *d )        {QT_PUT_TYPE(int)}
void qt_put_type_charp( void *hdl, int n, void * d )     {QT_PUT_TYPE(char *)}
void qt_put_type_long( void *hdl, int n, void *d )       {QT_PUT_TYPE(long)}
void qt_put_type_ulong( void *hdl, int n, void *d )      {QT_PUT_TYPE(unsigned long)}

void qt_put_fmt_char( void *hdl, void *dp, void *d )     {QT_PUT_FMT(char)}
void qt_put_fmt_u8( void *hdl, void *dp, void *d )       {QT_PUT_FMT(uint8_t)}
void qt_put_fmt_u16( void *hdl, void *dp, void *d )      {QT_PUT_FMT(uint16_t)}
void qt_put_fmt_u32( void *hdl, void *dp, void *d )      {QT_PUT_FMT(uint32_t)}
void qt_put_fmt_u64( void *hdl, void *dp, void *d )      {QT_PUT_FMT(uint64_t)}
void qt_put_fmt_s8( void *hdl, void *dp, void *d )       {QT_PUT_FMT(int8_t)}
void qt_put_fmt_s16( void *hdl, void *dp, void *d )      {QT_PUT_FMT(int16_t)}
void qt_put_fmt_s32( void *hdl, void *dp, void *d )      {QT_PUT_FMT(int32_t)}
void qt_put_fmt_s64( void *hdl, void *dp, void *d )      {QT_PUT_FMT(int64_t)}
void qt_put_fmt_bool( void *hdl, void *dp, void *d )     {QT_PUT_FMT(bool)}
void qt_put_fmt_double ( void *hdl, void *dp, void *d )  {QT_PUT_FMT(double)}
void qt_put_fmt_float ( void *hdl, void *dp, void *d )   {QT_PUT_FMT(float)}
void qt_put_fmt_int ( void *hdl, void *dp, void *d )     {QT_PUT_FMT(int)}
void qt_put_fmt_charp ( void *hdl, void *dp, void * d )  {QT_PUT_FMT(char *)}
void qt_put_fmt_long ( void *hdl, void *dp, void *d )    {QT_PUT_FMT(long)}
void qt_put_fmt_ulong ( void *hdl, void *dp, void *d )   {QT_PUT_FMT(unsigned long)}

}
