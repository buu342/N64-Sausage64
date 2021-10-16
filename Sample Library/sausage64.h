typedef struct {
    f32 pos[3];
    f32 rot[3];
    f32 scale[3];
} FrameData;

typedef struct {
    u16 framenumber;
    FrameData* framedata;
} KeyFrame;

typedef struct {
    const char* name;
    u16 framecount;
    KeyFrame* keyframes;
} Animation;

typedef struct {
    const char* name;
    Gfx* dl;
    Mtx matrix;
} Mesh;

typedef struct {
    u8 meshcount;
    u8 animcount;
    u16 activeframe;
    u32 animtick;
    void (*predraw)(u8);
    void (*postdraw)(u8);
    Mesh* meshes;
    Animation* anims;
} s64Model;