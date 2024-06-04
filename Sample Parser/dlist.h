#ifndef _SAUSN64_DLIST_H
#define _SAUSN64_DLIST_H

    #include <stdint.h>
    #include "mesh.h"
    #include "gbi.h"

    typedef struct {
        DListCName cmd;
        uint32_t size;
        uint32_t* data;
    } DLCBinary;

    extern uint16_t    swap_endian16(uint16_t val);
    extern uint32_t    swap_endian32(uint32_t val);
    extern float       swap_endianfloat(float val);
    extern linkedList* dlist_frommesh(s64Mesh* mesh, bool isbinary);
    extern void        construct_dltext();
    
#endif