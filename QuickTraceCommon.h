// Copyright (c) 2011 Arista Networks, Inc.  All rights reserved.
// Arista Networks, Inc. Confidential and Proprietary.

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

#define unlikely( cond ) __builtin_expect((cond),0)
#define likely( cond ) __builtin_expect((cond),1)

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
