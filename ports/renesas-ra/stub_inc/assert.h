/* Minimal stub assert.h for embedded - empty assert */
#ifndef _STUB_ASSERT_H
#define _STUB_ASSERT_H

/* Override any system assert with a no-op */
#ifdef assert
#undef assert
#endif
#define assert(x) ((void)0)

#endif
