#ifndef __ETCH_X86_HACK_H
#define __ETCH_X86_HACK_H

#ifdef ETCH_X86_HACK
//Etch has no htobe64 or be64toh
#include <bits/byteswap.h>
#define htobe64(x) __bswap_64(x)
#define be64toh(x) __bswap_64(x)
#endif

#endif

//IN GOD WE TRVST.
