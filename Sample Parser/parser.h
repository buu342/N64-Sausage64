#ifndef _SAUSN64_PARSER_H
#define _SAUSN64_PARSER_H

    /*********************************
               Custom Types
    *********************************/

    // Lexer state
    typedef enum {
        STATE_NONE,
        STATE_COMMENTBLOCK,
        STATE_MESH,
        STATE_VERTICES,
        STATE_FACES,
        STATE_ANIMATION,
        STATE_KEYFRAME
    } lexState;
    
    
    /*********************************
                Functions
    *********************************/

    extern void parse_sausage(FILE* fp);
    
#endif