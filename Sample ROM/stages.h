#ifndef UNFLEX_STAGES_H
#define UNFLEX_STAGES_H

    /*********************************
            Function Prototypes
    *********************************/

    void stage00_init();
    void stage00_update();
    void stage00_draw();
    
    char* command_setanim();
    char* command_listanims();
    char* command_setface();
    char* command_listfaces();
    char* command_togglelight();
    char* command_freezelight();
    
#endif