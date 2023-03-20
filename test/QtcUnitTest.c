// Copyright (c) 2018 Arista Networks, Inc.  All rights reserved.
// Arista Networks, Inc. Confidential and Proprietary.

#include <assert.h>
#include <stdlib.h>
#include <limits.h>
#include <time.h>
#include <QuickTrace/QuickTrace.h>
#define auto __extension__ __auto_type
#define unused __attribute__ ( ( unused ) )
#undef NDEBUG

#define FAIL_ON_QT_PUT_EXTENSIONS { \
      assert( 0 && "must be handled without extensions" ); }
#undef QT_PUT_EXTENSIONS
#define QT_PUT_EXTENSIONS( ... ) FAIL_ON_QT_PUT_EXTENSIONS

const char * const output_fname = "QtcUnitTest.qt";

static int
rand_range( int min, int max )
{
   assert( max <= RAND_MAX );
   int n = min + rand() % ( max - min + 1 );
   assert( n >= min && n <= max );
   return n;
}

#define ASSERT_AUTO_EQ_TYPEOF( T, expr ) do {                           \
   T v = expr;                                                          \
   auto unused auto_v = v;                                              \
   typeof( v ) typeof_v = v;                                            \
   assert( __builtin_types_compatible_p( typeof( auto_v ), typeof( typeof_v ) ) \
          && "checking " # T );                                          \
   } while ( 0 )

/*
 * Asserts that auto deducts the same type as typeof for non arrary types
 * Statically asserts that:
 * - typeof is not suitable for array types
 * - auto doesnt produce the same result as typeof
 * - auto produces a pointer type for a array initializer
 */
static void
test_auto_matches_typeof()
{
   char char_array[] = "any";
   typeof( char_array ) typeof_char_array;
   auto auto_char_ptr = char_array;

   static_assert( __builtin_types_compatible_p( typeof( char_array ),
                                                typeof( typeof_char_array ) ),
                  "we cannot initialize typeof_char_array with char_array" );
   static_assert( ! __builtin_types_compatible_p( typeof( char_array ),
                                                  typeof( auto_char_ptr ) ),
                  "auto does not declare a char array" );
   static_assert( __builtin_types_compatible_p( typeof( auto_char_ptr ),
                                                char * ),
                  "hence we can initialize auto_char_ptr with char_array" );

   ASSERT_AUTO_EQ_TYPEOF( char, 0 );
   ASSERT_AUTO_EQ_TYPEOF( double, 0 );
   ASSERT_AUTO_EQ_TYPEOF( float, 0 );
   ASSERT_AUTO_EQ_TYPEOF( int8_t, 0 );
   ASSERT_AUTO_EQ_TYPEOF( int16_t, 0 );
   ASSERT_AUTO_EQ_TYPEOF( int32_t, 0 );
   ASSERT_AUTO_EQ_TYPEOF( int64_t, 0 );
   ASSERT_AUTO_EQ_TYPEOF( uint8_t, 0 );
   ASSERT_AUTO_EQ_TYPEOF( uint16_t, 0 );
   ASSERT_AUTO_EQ_TYPEOF( uint32_t, 0 );
   ASSERT_AUTO_EQ_TYPEOF( uint64_t, 0 );
   ASSERT_AUTO_EQ_TYPEOF( bool, 0 );
   ASSERT_AUTO_EQ_TYPEOF( const char *, auto_char_ptr );
   ASSERT_AUTO_EQ_TYPEOF( char *, auto_char_ptr );
   ASSERT_AUTO_EQ_TYPEOF( unsigned int, 0 );
   ASSERT_AUTO_EQ_TYPEOF( int, 0 );
   ASSERT_AUTO_EQ_TYPEOF( long, 0 );
   ASSERT_AUTO_EQ_TYPEOF( unsigned long, 0 );
   ASSERT_AUTO_EQ_TYPEOF( void *, auto_char_ptr );
}

/*
 * Check if quicktrace evaluates a parameter more than once
 * causing unexpected side effects.
 */
static void
test_single_evaluation()
{
   int counter = 0;
   int repeat = rand_range( 10, 1000 );

   for ( int i = 0; i < repeat; i++ ) {
      QTRACEF0( "expression evaluation counter: %d", counter++ );
   }

   assert( counter == repeat );
}


#define QTRACE_TYPE( T, expr ) do { \
      T v = ( T ) ( expr );         \
      QTRACEF0( #T, v );            \
   } while ( 0 )                    \

/*
 * asserts we successfully compile each type's use
 * and generates output to be checked later
 */
static void
generate_qtrace_types_output()
{
   size_t i = rand_range( 3, 127 );
   char s[] = { i, 0 };

   QTRACEF0( "char[]", s );
   QTRACE_TYPE( char, i );
   QTRACE_TYPE( double, i );
   QTRACE_TYPE( float, i );
   QTRACE_TYPE( int8_t, i );
   QTRACE_TYPE( int16_t, i );
   QTRACE_TYPE( int32_t, i );
   QTRACE_TYPE( int64_t, i );
   QTRACE_TYPE( uint8_t, i );
   QTRACE_TYPE( uint16_t, i );
   QTRACE_TYPE( uint32_t, i );
   QTRACE_TYPE( uint64_t, i );
   QTRACE_TYPE( bool, i % 2 );
   QTRACE_TYPE( const char *, s );
   QTRACE_TYPE( char *, s );
   QTRACE_TYPE( unsigned int, i );
   QTRACE_TYPE( int, i );
   QTRACE_TYPE( long, i );
   QTRACE_TYPE( unsigned long, i );
   QTRACE_TYPE( void *, i );
}

/*
 * Check if the QT_PUT_EXTENSIONS hook is being used for a unknown type
 */
static void
test_qt_put_extensions()
{
   bool passed = false;
   struct {} dummy;

#undef QT_PUT_EXTENSIONS
#define QT_PUT_EXTENSIONS( ... ) { passed = \
         __builtin_types_compatible_p( typeof( _qt_tmp ), typeof( dummy ) ); }

   QTRACEF0( "extension", dummy );

#undef QT_PUT_EXTENSIONS
#define QT_PUT_EXTENSIONS FAIL_ON_QT_PUT_EXTENSIONS

   assert( passed && "QT_PUT_EXTENSIONS must be used" );
}


static void
set_up()
{
   char *seed_str = getenv( "ARTEST_RANDSEED" );
   srand( seed_str ? strtoll( seed_str, NULL, 10 ) : time( NULL ) );
   qt_initialize_default_handle( output_fname,
                                 "16, 16, 16, 16, 16, 16, 16, 16, 16, 16" );
}

int main( int argc, const char *argv[] )
{
   set_up();
   test_auto_matches_typeof();
   test_qt_put_extensions();
   test_single_evaluation();
   generate_qtrace_types_output();
   return 0;
}
