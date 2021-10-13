#ifndef _SAUSN64_MAIN_H
#define _SAUSN64_MAIN_H

    #include "datastructs.h"
    
    
    /*********************************
                 Globals
    *********************************/
    
    extern linkedList list_meshes;
    extern linkedList list_animations;
    extern linkedList list_textures;
    extern linkedList list_output;
    
    extern bool global_quiet;
    extern bool global_fixroot;
    extern bool global_usetextures;
    extern bool global_binaryout;
    extern bool global_vertexcolors;
    extern char* global_outputname;
    extern char* global_modelname;
    extern unsigned int global_cachesize;
    
    
    /*********************************
                Functions
    *********************************/

    extern void terminate(char* message);
    
#endif