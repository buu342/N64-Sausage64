#ifndef UNFLEX_STAGES_H
#define UNFLEX_STAGES_H

    /*********************************
            Function Prototypes
    *********************************/

    // Stage functions
    void stage00_init();
    void stage00_update();
    void stage00_draw();
    
    // USB functions
    char* command_setanim();
    char* command_listanims();
    char* command_setface();
    char* command_listfaces();
    char* command_togglelight();
    char* command_freezelight();
    char* command_togglelerp();
    char* command_toggleloop();
    char* command_toggleaxis();
    char* command_togglelookat();
    
#endif