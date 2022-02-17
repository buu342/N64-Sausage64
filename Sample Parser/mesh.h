#ifndef _SAUSN64_MESH_H
#define _SAUSN64_MESH_H

    /*********************************
                 Structs
    *********************************/

    // Mesh struct
    typedef struct {
        char* name;
        Vector3D root;
        linkedList verts;
        linkedList faces;
        linkedList props;
    } s64Mesh;
    
    // Vertex struct
    typedef struct {
        Vector3D pos;
        Vector3D normal;
        Vector3D color;
        Vector2D UV;
    } s64Vert;
    
    // Face struct
    typedef struct {
        int vertcount;
        unsigned int verts[6];
        n64Texture* texture;
    } s64Face;
    
    
    /*********************************
                Functions
    *********************************/
    
    extern s64Mesh* add_mesh(char* name);
    extern s64Vert* add_vertex(s64Mesh* mesh);
    extern s64Face* add_face(s64Mesh* mesh);
    extern s64Mesh* find_mesh(char* name);
    
#endif