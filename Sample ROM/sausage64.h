typedef struct {
    f32 pos[3];
    f32 rot[3];
    f32 scale[3];
} FrameData;

typedef struct {
    u32 framenumber;
    FrameData* framedata;
} KeyFrame;

typedef struct {
    const char* name;
    u32 framecount;
    KeyFrame* keyframes;
} Animation;

typedef struct {
    const char* name;
    Gfx* dl;
} Mesh;

typedef struct {
    u16 meshcount;
    u16 animcount;
    Mesh* meshes;
    Animation* anims;
} s64ModelData;

typedef struct {
    u8 interpolate;
    u8 loop;
    u32 curframeindex;
    OSTime nexttick;
    Animation* curanim;
    u32 animtick;
    Mtx* matrix;
    void (*predraw)(u16);
    void (*postdraw)(u16);
    s64ModelData* mdldata;
} s64ModelHelper;

extern void sausage64_initmodel(s64ModelHelper* mdl, s64ModelData* mdldata, Mtx* matrices);
extern void sausage64_set_anim(s64ModelHelper* mdl, u16 anim);
extern void sausage64_set_predrawfunc(s64ModelHelper* mdl, void (*predraw)(u16));
extern void sausage64_set_postdrawfunc(s64ModelHelper* mdl, void (*postdraw)(u16));
extern void sausage64_advance_anim(s64ModelHelper* mdl);
extern void sausage64_drawmodel(Gfx** glistp, s64ModelHelper* mdl);