#ifndef _SAUSN64_DLIST_H
#define _SAUSN64_DLIST_H

    #include <stdint.h>
    #include "mesh.h"

    extern uint16_t    swap_endian16(uint16_t val);
    extern uint32_t    swap_endian32(uint32_t val);
    extern linkedList* dlist_frommesh(s64Mesh* mesh, bool isbinary);
    extern void        construct_dltext();
    
#endif