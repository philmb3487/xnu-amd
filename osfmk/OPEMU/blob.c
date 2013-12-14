/** Sinetek:
 *
 *  This file is meant to present the compiler with
 *  a single compilation unit for the entire OPEMU.
 *  This is done in order to simplify things a little
 *  bit:  to reduce the need for Makefiles and so on.
 *
 *  TODO: get rid of all warnings
 **/

#include "opemu.c"
#include "sse3x.c"

/*** bring in libudis86 static ***/

#include "libudis86/decode.c"
#include "libudis86/itab.c"
//#include "libudis86/syn-att.c"
#include "libudis86/syn-intel.c"
#include "libudis86/syn.c"
#include "libudis86/udis86.c"

