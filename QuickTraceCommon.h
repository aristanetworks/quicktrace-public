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

#ifndef QUICKTRACE_COMMON_H_
#define QUICKTRACE_COMMON_H_

#include <stdio.h>
#include <stdbool.h>
#include <alloca.h>
#include <stdint.h>
#include <string.h>

#ifdef QT_RDTSC_MOCK
extern uint64_t qt_rdtsc_mocker;
#endif

#define QUICKTRACE_UNLIKELY( cond ) __builtin_expect((cond),0)
#define QUICKTRACE_LIKELY( cond ) __builtin_expect((cond),1)

// Unsynchronized rdtsc (synchronizing would require a cpuid or other
// memory barrier, which is slow).  Being unsynchronized means that
// our snapshot of the cycle count might be reordered relative to
// surrounding instructions, but for this usage, that's not a problem
// since a slightly inaccurate time at this level is fine.
static inline uint64_t rdtsc( void )
#ifdef __cplusplus
   noexcept
#endif // __cplusplus
{
#ifdef QT_RDTSC_MOCK
   // tests define their qt_rdtsc_mocker
   return ++qt_rdtsc_mocker;
#elif defined( __x86_64__ ) || defined( __i386__ )
   uint32_t a, d;
   __asm__ __volatile__( "rdtsc" : "=a"( a ), "=d"( d ) );
   return ( ( uint64_t )d << 32 ) | ( uint64_t )a;
#elif defined( __aarch64__ )
   // arm64 doesn't have an identical register to rdtsc but
   // cntvct_el0 is close enough, main issue is that it is only
   // 56 bits long on some older systems so could rollover
   uint64_t rc;
   asm volatile( "mrs %0, cntvct_el0" : "=r"( rc ) );
   return rc;
#else
#error "Add rdtsc() equivalent for your arch"
#endif
}

typedef struct qtprof_t_ {
  void *th;
  int mid;                                                           
  uint64_t tsc;                                                      
} qtprof_t; 

#endif
// end-moss-ignore
