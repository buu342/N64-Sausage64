typedef struct {
    float pos[3];
    float rot[3];
    float scale[3];
} FrameData;

typedef struct {
    int framenumber;
    int meshcount;
    FrameData* framedata;
} KeyFrame;

typedef struct {
    char* name;
    int framecount;
    KeyFrame* keyframes;
} Animation;

typedef struct {
    int animcount;
    Animation* anim;
} AnimList;

typedef struct {
    char* name;
    Gfx* dl;
} Mesh;

typedef struct {
    int meshcount;
    Mesh* mesh;
} MeshList;