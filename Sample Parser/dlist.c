/***************************************************************
                            dlist.c
                             
Constructs a display list string for outputting later.
To optimize the triangle list, I utilize the Forsyth algorithm,
heavily basing my code off the implementation by Martin Strosjo, 
available here: http://www.martin.st/thesis/forsyth.cpp
A lot of loops can be cleaned up in the future... There's too 
many unecessary ones...
***************************************************************/

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <math.h>
#include "main.h"
#include "texture.h"
#include "mesh.h"


/*********************************
              Macros
*********************************/

#define STRBUF_SIZE 512

#define FORSYTH_SCORE_SCALING 7281
#define FORSYTH_CACHE_DECAY_POWER 1.5
#define FORSYTH_LAST_TRI_SCORE 0.75
#define FORSYTH_VALENCE_BOOST_SCALE 2.0
#define FORSYTH_VALENCE_BOOST_POWER 0.5


/*********************************
             Structs
*********************************/

// Vertex cache block
typedef struct {
    int offset;
    int reuse;
    linkedList verts;
    linkedList faces;
} vertCache;


/*********************************
             Globals
*********************************/

static int* forsyth_posscore = NULL;
static int* forsyth_valencescore = NULL;


/*==============================
    split_verts_by_texture
    Takes in a mesh and splits the vertices and triangles by texture
    into separate vertex cache structs
    @param The mesh to parse
    @param The vertex cache list to store into
==============================*/

static void split_verts_by_texture(s64Mesh* mesh, linkedList* vcache)
{
    hashTable vc_vertstable;
    listNode* facenode = mesh->faces.head;
    n64Texture* curtex = NULL;
    vertCache* curvc = NULL;
    int offset = 0;
    memset(&vc_vertstable, 0, sizeof(vc_vertstable));

    // Go through all the faces so that we can see what textures they're using
    while (facenode != NULL)
    {
        int i;
        s64Face* face = (s64Face*)facenode->data;
        
        // If we found a new texture, create a new vertex cache
        if (face->texture != curtex) // TODO: Potential problem, NULL texture because of "None" material name?
        {
            curtex = face->texture;
            
            // Destroy the vertex hashtable if it isn't empty
            if (vc_vertstable.size != 0)
                htable_destroy(&vc_vertstable);
                
            // Calculate the new offset
            if (curvc != NULL)
                offset += curvc->verts.size;
            
            // Create a new vertex cache struct and append it to our list
            vertCache* vc = (vertCache*) calloc(1, sizeof(vertCache));
            if (vc == NULL)
                terminate("Error: Unable to allocate memory for vertex cache\n");
            vc->offset = offset;
            list_append(vcache, vc);
            curvc = vc;
        }
        
        // Add this vert to our list if it's not there yet
        for (i=0; i<face->vertcount; i++)
        {
            int vindex = face->verts[i];
            if (htable_getkey(&vc_vertstable, vindex) == NULL)
            {
                s64Vert* vert = (s64Vert*) (list_node_from_index(&mesh->verts, vindex)->data);
                htable_append(&vc_vertstable, vindex, vert);
                list_append(&curvc->verts, vert);
            }
        }
        
        // Add this face to the vertex cache list, and then check the next face
        list_append(&curvc->faces, face);
        facenode = facenode->next;
    }

    // Clean up memory we've used up
    if (vc_vertstable.size != 0)
        htable_destroy(&vc_vertstable);
}


/*==============================
    offset_vertindices
    Fixes the offset in a list of faces
    @param The list of faces
    @param The offset starting value
==============================*/

static void offset_vertindices(linkedList* faces, int offset)
{
    listNode* facenode = faces->head;
    hashTable vertstable;
    int vertcount = 0;
    memset(&vertstable, 0, sizeof(vertstable));
    
    while (facenode != NULL)
    {
        int i;
        s64Face* face = (s64Face*)facenode->data;
        
        // Go through all the verts
        for (i=0; i<face->vertcount; i++)
        {
            dictNode* hkeyval = htable_getkey(&vertstable, face->verts[i]);
        
            // If this vertex index isn't in our hashtable yet
            if (hkeyval == NULL)
            {
                hkeyval = htable_append(&vertstable, face->verts[i], (int*)(offset+vertcount)); // It's not a pointer, I'm lying
                vertcount++;
            }
            
            // Fix the vert index from the hashtable value
            face->verts[i] = (int) hkeyval->value;
        }

        // Fix the next face
        facenode = facenode->next;
    }
    
    // Clean up memory we've used up
    if (vertstable.size != 0)
        htable_destroy(&vertstable);
}


/*==============================
    combine_caches
    Attempts to combine vertex caches
    Ineffient since it's O(N^2)
    @param the vertex cache list to combine
==============================*/

static void combine_caches(linkedList* vcachelist)
{
    int offset = 0;
    listNode* firstnode = vcachelist->head;
    listNode* secondnode = vcachelist->head;
    
    // Iterate through all the nodes to check which can be combines
    while (firstnode != NULL)
    {
        vertCache* vc1 = (vertCache*)firstnode->data;
        
        // Now look through all the other elements (double for loop style)
        while (secondnode != NULL)
        {
            vertCache* vc2 = (vertCache*)secondnode->data;
            
            // If we're not looking at the same object
            if (vc1 != vc2)
            {
                // If these two together would fit in the chace, then combine them
                if (vc1->verts.size + vc2->verts.size <= global_cachesize)
                {
                    // Fix vertex offsets in the new vertex list and then combine the lists
                    offset_vertindices(&vc2->faces, vc1->verts.size);
                    list_combine(&vc1->verts, &vc2->verts);
                    list_combine(&vc1->faces, &vc2->faces); 
                    list_remove(vcachelist, vc2);
                }
            }
            secondnode = secondnode->next;
        }
        firstnode = firstnode->next;
        secondnode = vcachelist->head;
    }
    
    // Recalculate all the offsets (needs to be done anyway since split_verts_by_texture() doesn't)
    firstnode = vcachelist->head;
    while (firstnode != NULL)
    {
        vertCache* vcfix = (vertCache*)firstnode->data;
        vcfix->offset = offset;
        offset += vcfix->verts.size;
        offset_vertindices(&vcfix->faces, 0);
        firstnode = firstnode->next;
    }
}


/*==============================
    forsyth_init
    Initialize the global Forsyth score lookup tables
==============================*/

static inline void forsyth_init()
{
    int i;
    
    // Allocate memory for the cache position score array
    if (forsyth_posscore == NULL)
    {
        forsyth_posscore = (int*)malloc(sizeof(int)*global_cachesize);
        if (forsyth_posscore == NULL)
            terminate("Unable to allocate memory for forsyth cache pos score\n");
    }
    
    // Allocate memory for the valence score array
    if (forsyth_valencescore == NULL)
    {
        forsyth_valencescore = (int*)malloc(sizeof(int)*global_cachesize);
        if (forsyth_valencescore == NULL)
            terminate("Unable to allocate memory for forsyth valence score\n");
    }
    
    // Precalculate the position score array
    for (i=0; i<global_cachesize; i++)
    {
        float score = 0;
        if (i >= 3)
        {
            const float scaler = 1.0/(global_cachesize - 3);
            score = 1.0 - (i - 3)*scaler;
            score = powf(score, FORSYTH_CACHE_DECAY_POWER);
        }
        else
            score = FORSYTH_LAST_TRI_SCORE;
        forsyth_posscore[i] = (FORSYTH_SCORE_SCALING * score);
    }

    // Precalculate the valence score array
    for (i=1; i<global_cachesize; i++)
    {
        float valenceboost = powf(i, -FORSYTH_VALENCE_BOOST_POWER);
        float score = FORSYTH_VALENCE_BOOST_SCALE*valenceboost;
        forsyth_valencescore[i] = (FORSYTH_SCORE_SCALING*score);
    }
}


/*==============================
    forsyth_calcvertscore
    Calculates the heuristic for a vertex
    @param The amount of triangles shared by this vertex
    @param the cache position of this vertex
    @returns The calculated score
==============================*/

static int forsyth_calcvertscore(int verttris, int cachepos)
{
    int score = 0;

    // If the vertex is not in any triangle, then it gets no score
    if (verttris == 0)
        return 0;

    // If the vertex is in the cache, give it a score based on its position
    if (cachepos >= 0)
        score = forsyth_posscore[cachepos];

    // Add to the score based on how many triangles this vert is sharing
    if (verttris < global_cachesize)
        score += forsyth_valencescore[verttris];
    return score;
}


/*==============================
    forsyth
    Applies the forsyth algorithm for organizing vertices
    to prevent cache misses.
    https://tomforsyth1000.github.io/papers/fast_vert_cache_opt.html
    @param The vertex cache to optimize
    @returns A list of vertex caches, split to fit the cache limit
==============================*/

static linkedList* forsyth(vertCache* vcache)
{
    int i, j;
    int *indices, *activetricount;
    int curtri = 0, tricount = 0, sum = 0, outpos = 0, scanpos = 0, besttri = -1, bestscore = -1;
    int vertcount = vcache->verts.size;
    listNode* curnode = vcache->faces.head;
    int* offsets, *lastscore, *cachetag, *triscore, *triindices, *outtris, *outindices, *tempcache;
    bool* triadded;
    int largestvertindex = 0, lowestvertindex = 0, vertindexcount = 0, lastcachesplit = 0;
    hashTable vertstable;
    linkedList newvertorder = {0, NULL, NULL};
    vertCache* curvcache = NULL;
    n64Texture* meshtex;
    linkedList* newvcachelist;
    
    // State we're using Forsyth
    printf("Applying Forsyth to cache block\n");
    
    // Calculate the number of triangles
    while (curnode != NULL)
    {
        s64Face* face = (s64Face*) curnode->data;
        tricount++;
        if (face->vertcount == 4)
            tricount++;
        curnode = curnode->next;
    }
    
    // Allocate memory for the vertex indices list
    indices = (int*) calloc(1, sizeof(int)*tricount*3);
    outindices = (int*) calloc(1, sizeof(int)*tricount*3); // To be removed later
    if (indices == NULL)
        terminate("Error: Unable to allocate memory for vertex indices list\n");
    
    // Allocate memory for the vertex triangle count list
    activetricount = (int *)calloc(1, sizeof(int) * vertcount);
    if (activetricount == NULL)
        terminate("Error: Unable to allocate memory for vertex triangle count\n");
    
    // Generate a list of vertex triangle indices
    curnode = vcache->faces.head;
    while (curnode != NULL)
    {
        s64Face* face = (s64Face*) curnode->data;
        for (i=0; i<3; i++)
            indices[curtri*3+i] = face->verts[i];
        curtri++;
        if (face->vertcount == 4)
        {
            indices[curtri*3] = face->verts[0];
            indices[curtri*3+1] = face->verts[2];
            indices[curtri*3+2] = face->verts[3];
            curtri++;
        }
        curnode = curnode->next;
    }
    
    
    /* ---------- Forsyth starts here ---------- */
    
    // Go through the vertex data and count all the ocurrences of a given vertex
    for (i=0; i<3*tricount; i++)
        activetricount[indices[i]]++;
    
    // Allocate all the memory we need
    offsets = (int*) calloc(1, sizeof(int)*vertcount);
    lastscore = (int*) calloc(1, sizeof(int)*vertcount);
    cachetag = (int*) calloc(1, sizeof(int)*vertcount);
    triscore = (int*) calloc(1, sizeof(int)*tricount);
    triindices = (int*) calloc(1, sizeof(int)*3*tricount);
    outtris = (int*)calloc(1, sizeof(int)*tricount);
    tempcache = (int*)malloc(sizeof(int)*(global_cachesize+3));
    triadded = (bool*) calloc(1, sizeof(bool)*tricount);
    if (offsets == NULL || lastscore == NULL || cachetag == NULL || triscore == NULL || triindices == NULL || outtris == NULL || tempcache == NULL || triadded == NULL)
        terminate("Error: Failure allocating memory for Forsyth algorithm\n");
        
    // Count the triangle array offset for each vertex
    for (i=0; i<vertcount; i++) 
    {
        offsets[i] = sum;
        sum += activetricount[i];
        activetricount[i] = 0;
        cachetag[i] = -1;
    }
    
    // Fill the vertex data structures with indices to the triangles using each vertex
    for (i=0; i<tricount; i++)
    {
        for (j=0; j<3; j++)
        {
            int v = indices[3*i + j];
            triindices[offsets[v] + activetricount[v]] = i;
            activetricount[v]++;
        }
    }

    // Initialize the score for all vertices
    for (i=0; i<vertcount; i++)
    {
        lastscore[i] = forsyth_calcvertscore(activetricount[i], cachetag[i]);
        for (j=0; j<activetricount[i]; j++)
            triscore[triindices[offsets[i] + j]] += lastscore[i];
    }
    
    // Find the best triangle
    for (i=0; i<tricount; i++)
    {
        if (triscore[i] > bestscore)
        {
            bestscore = triscore[i];
            besttri = i;
        }
    }
    
    // Output the best triangle, as long as there are triangles left to output
    memset(tempcache, -1, sizeof(int)*(global_cachesize+3));
    while(besttri >= 0)
    {
        // Mark this triangle as added
        triadded[besttri] = TRUE;
        
        // Output this triangle
        outtris[outpos++] = besttri;
        for (i=0; i<3; i++)
        {
            // Update this vertex
            int v = indices[3*besttri + i];

            // Check the current cache position, if it is in the cache
            if (cachetag[v] < 0)
                cachetag[v] = global_cachesize + i;
            if (cachetag[v] > i)
            {
                // Move all cache entries from the previous position in the cache to the new target position
                for (j=cachetag[v]; j>i; j--)
                {
                    tempcache[j] = tempcache[j - 1];
                    
                    // If this cache slot contains a real vertex, update its cache tag
                    if (tempcache[j] >= 0)
                        cachetag[tempcache[j]]++;
                }
                
                // Insert the current vertex into its new target slot
                tempcache[i] = v;
                cachetag[v] = i;
            }

            // Find the current triangle in the list of active triangles and remove it
            for (j=0; j<activetricount[v]; j++)
            {
                if (triindices[offsets[v] + j] == besttri) 
                {
                    triindices[offsets[v] + j] = triindices[offsets[v] + activetricount[v] - 1];
                    break;
                }
            }
            
            // Shorten the list
            activetricount[v]--;
        }
        
        // Update the scores of all triangles in the cache
        for (i=0; i<global_cachesize+3; i++)
        {
            int newscore, diff;
            int v = tempcache[i];
            
            // Skip if the vertex isn't in the cache
            if (v < 0)
                break;
                
            // This vertex has been pushed outside of the actual cache
            if (i >= global_cachesize)
            {
                cachetag[v] = -1;
                tempcache[i] = -1;
            }
            
            // Calculate the new score
            newscore = forsyth_calcvertscore(activetricount[v], cachetag[v]);
            diff = newscore - lastscore[v];
            for (j=0; j<activetricount[v]; j++)
                triscore[triindices[offsets[v] + j]] += diff;
            lastscore[v] = newscore;
        }
        
        // Find the best triangle referenced by vertices in the cache
        besttri = -1;
        bestscore = -1;
        for (i=0; i<global_cachesize; i++)
        {
            int v = tempcache[i];
            
            // Skip if the vertex isn't in the cache
            if (v < 0)
                break;
                
            // Search for the best triangle
            for (j=0; j<activetricount[v]; j++)
            {
                int t = triindices[offsets[v] + j];
                if (triscore[t] > bestscore)
                {
                    besttri = t;
                    bestscore = triscore[t];
                }
            }
        }
        
        // If no active triangle was found, continue searching through the list of triangles until we find another best triangle
        if (besttri < 0) 
        {
            while (scanpos < tricount)
            {
                if (!triadded[scanpos])
                {
                    besttri = scanpos;
                    break;
                }
                scanpos++;
            }
        }
    }
    
    // Convert the triangle index array into a full triangle list
    outpos = 0;
    for (i=0; i<tricount; i++)
    {
        for (j=0; j<3; j++)
        {
            int v = indices[3*outtris[i] + j];
            outindices[outpos++] = v;
        }
    }
    
    /* ---------- Forsyth ends here ---------- */
    

    // Now that we have an optimized vert list, what we want to do is the following:
    // * Create a list of vertices with the new vertex order
    // * Fix all the triangle indices to start from 0
    // * Keep adding faces to a cache until we hit our vertex limit 
    // * In the next cache block, we might need to correct the offset if an earlier vertex shows up again
    //   Example, we're loading the second block of verts (which starts at pos 32), but one triangle requires
    //   the vert with index 28, the offset here is -4 of what the block's offset should be.
    // * Once that's done, we add the vertices to each cache block while taking into account the offset to ensure
    //   we don't add duplicated vertices (because they're in a different cache block).
    
        
    // Now that we have an optimized list of verts, lets create a linked list with the verts in the new order
    j = 0;
    memset(&vertstable, 0, sizeof(vertstable));
    for (i=0; i<tricount*3; i++)
    {
        dictNode* hkeyval = htable_getkey(&vertstable, outindices[i]);
    
        // If this vertex index isn't in our hashtable yet, add it
        if (hkeyval == NULL)
        {
            hkeyval = htable_append(&vertstable, outindices[i], (int*)(j++)); // It's not a pointer, I'm lying
            list_append(&newvertorder, list_node_from_index(&vcache->verts, outindices[i]));
        }
        
        // Fix the vert index from the hashtable value
        outindices[i] = (int) hkeyval->value;
    }
    htable_destroy(&vertstable);
    
    // Allocate memory for our new vertex cache list
    newvcachelist = (linkedList*) calloc(1, sizeof(linkedList));
    if (newvcachelist == NULL)
        terminate("Error: Unable to allocate memory for new vertex cache list\n");
    
    // Now iterate through the optimized list again, start generating triangles and adding them to our cache list
    meshtex = ((s64Face*)(vcache->faces.head))->texture; // Since this mesh is already split by texture, we can rely on the first face's texture applying to all faces
    for (i=0; i<tricount*3; i+=3)
    {
        s64Face* face;
    
        // If we don't have a current vertex cache, create a new one and add it to our list
        if (curvcache == NULL)
        {
            curvcache = (vertCache*) calloc(1, sizeof(vertCache));
            if (curvcache == NULL)
                terminate("Error: Unable to allocate memory for corrected vertex cache\n");
            list_append(newvcachelist, curvcache);
        }

        // Update the largest and lowest vertex tracker
        for (j=i; j<i+3; j++)
        {
            if (largestvertindex < outindices[j])
            {
                largestvertindex = outindices[j];
                vertindexcount++;
                if (vertindexcount+(-curvcache->offset) >= global_cachesize)
                    break;
            }
            else if (lowestvertindex > outindices[j])
            {
                lowestvertindex = outindices[j];
                curvcache->offset = lowestvertindex-lastcachesplit; // Should give a negative number
                if (vertindexcount+(-curvcache->offset) >= global_cachesize)
                    break;
            }
        }
            
        // If adding this vertex exceeds our cache size, restart this loop so we create a new cache
        if (vertindexcount+(-curvcache->offset) >= global_cachesize)
        {
            lastcachesplit += global_cachesize;
            vertindexcount = 0;
            lowestvertindex = largestvertindex;
            curvcache = NULL;
            i -= 3;
            // TODO: probably have an infinite loop check here that terminates the program if this mesh can't be split further
            continue;
        }
     
        // If we didn't run into trouble, we can add a triangle to this vertex cache!
        face = (s64Face*) calloc(1, sizeof(s64Face));
        face->vertcount = 3;
        face->verts[0] = outindices[i];
        face->verts[1] = outindices[i+1];
        face->verts[2] = outindices[i+2];
        face->texture = meshtex;
        list_append(&curvcache->faces, face);
    }
    
    // Now that we have good cache blocks with the faces added in, last thing we need to do is add the correct vertices and update all the indices one last time
    curnode = newvcachelist->head;
    lastcachesplit = 0;
    while (curnode != NULL)
    {
        vertCache* curvc = (vertCache*)curnode->data;
        listNode* facenode = curvc->faces.head;
        
        // Iterate through the faces
        while (facenode != NULL)
        {
            s64Face* face = (s64Face*)facenode->data;
            
            // Go through each vertex in this face
            for (i=0; i<face->vertcount; i++)
            {
                dictNode* hkeyval = htable_getkey(&vertstable, face->verts[i]);
                
                // If we haven't added this vert to this cache block, do so
                if (hkeyval == NULL)
                {
                    s64Vert* vert = (s64Vert*) (list_node_from_index(&newvertorder, face->verts[i])->data);
                    htable_append(&vertstable, face->verts[i], vert);
                    
                    // Add to the vert list if this vert was not in the previous cache block (IE it has a positive offset)
                    if (face->verts[i] - lastcachesplit >= 0)
                        list_append(&curvc->verts, vert);
                }
                    
                // Correct the vertex index to start from zero
                face->verts[i] = face->verts[i] + (-curvc->offset) - lastcachesplit;
            }
            facenode = facenode->next;
        }

        // Go to the next cache block
        htable_destroy(&vertstable);
        lastcachesplit += global_cachesize;
        curnode = curnode->next;
    }
        
    // Free all the memory used by the algorithm
    list_destroy(&newvertorder);
    free(indices);
    free(activetricount);
    free(offsets);
    free(outtris);
    free(tempcache);
    free(lastscore);
    free(cachetag);
    free(triscore);
    free(triindices);
    free(triadded);
    return newvcachelist;
}


/*==============================
    check_splitverts
    Go through a vertex cache list and check if they need to be split further
    @param The list of vertex caches
==============================*/

static void check_splitverts(linkedList* vcachelist)
{
    int newoffset = 0;
    int nindex = 0;
    listNode* curnode = vcachelist->head;
    while (curnode != NULL)
    {
        vertCache* vc = (vertCache*) curnode->data;
        if (vc->verts.size > global_cachesize)
        {
            // Use forsyth to generate a new list of cache blocks
            linkedList* newvclist = forsyth(vc);
            listNode* newvcnode = newvclist->head;
            
            // We need to correct all the offsets from our old value
            newoffset = vc->offset;
            while (newvcnode != NULL)
            {
                vertCache* newvc = (vertCache*)newvcnode->data;
                newvc->reuse = -newvc->offset;
                newvc->offset += newoffset;
                newoffset += newvc->verts.size;
                newvcnode = newvcnode->next;
            }
            
            // Remove the old vertex cache from the list and append our new list
            list_swapindex_withlist(vcachelist, nindex, newvclist);
            
            // Clean up allocated memory
            free(newvclist);
        }
        nindex++;
        curnode = curnode->next;
    }
}


/*==============================
    find_facefromvertindex
    Returns the first face in a list of faces that has a vert with the given index
    @param The list of faces
    @param The vertex index
    @returns The FIRST face with the given vertex index, or NULL if none is found
==============================*/

static s64Face* find_facefromvertindex(linkedList* faces, int index)
{
    listNode* curnode = faces->head;
    
    // Iterate through the face list
    while (curnode != NULL)
    {
        int i;
        s64Face* face = (s64Face*)curnode->data;
        
        // Check if the index is in this face
        for (i=0; i<face->vertcount; i++)
            if (face->verts[i] == index)
                return face;
            
        // If it isn't, check the next face
        curnode = curnode->next;
    }
    
    // No face found, return NULL
    return NULL;
}


/*==============================
    newstring
    Allocates memory for a string
    @param The string to duplicate
    @returns The duplicated string
==============================*/

static char* newstring(char* string)
{
    char* str = (char*) malloc(strlen(string)+1);
    if (str == NULL)
        terminate("Error: Unable to allocate memory for output string");
    strcpy(str, string);
    return  str;
}


/*==============================
    construct_dl
    Constructs a display list into a series of strings to output later.
    This method a bit ugly, in the future I'd change this to output to a file
    instead, and then have the write_output_X() functions combine them.
==============================*/

void construct_dl()
{
    int vcblockvertcount;
    char strbuff[STRBUF_SIZE];
    listNode* curmesh = list_meshes.head;
    n64Texture* lastTexture = NULL;
    
    // Precalculate forsyth tables as we might need them
    forsyth_init();
    
    // Begin display list construction
    if (!global_quiet) printf("*Constructing display lists\n");
    
    // Vertex data header
    list_append(&list_output, "\n// Custom combine mode to allow mixing primitive and vertex colors\n"
                              "#define G_CC_PRIMLITE SHADE,0,PRIMITIVE,0,0,0,0,PRIMITIVE\n\n\n"
                              "/*********************************\n"
                              "              Models\n"
                              "*********************************/\n\n"
    );

    // Iterate through each mesh
    while (curmesh != NULL)
    {
        s64Mesh* cmesh = (s64Mesh*)curmesh->data;
        linkedList vcache = {0, NULL, NULL};
        int vertindex = 0;
        listNode* vcnode;
        
        // If the model will not fit in our vertex cache, we must use split it
        if (cmesh->verts.size > global_cachesize)
        {
            if (!global_quiet) printf("Optimizing '%s' for vertex cache size\n", cmesh->name);
            
            // First, split everything by texture and see if that improves our situation
            split_verts_by_texture(cmesh, &vcache);
            
            // Try to combine any cache blocks that could fit together
            combine_caches(&vcache);
            
            // If that didn't help, then split the vertex block further with the help of Forsyth
            check_splitverts(&vcache);
            
            // TODO: If that still didn't help, then duplicate problem vertices and try Forsyth again
        }
        else
        {
            // Otherwise, just feed this block the vertex and face list directly from the mesh
            vertCache* vc = (vertCache*) malloc(sizeof(vertCache));
            if (vc == NULL)
                terminate("Error: Unable to allocate memory for vertex cache\n");
            vc->offset = 0;
            vc->verts = cmesh->verts;
            vc->faces = cmesh->faces;
            list_append(&vcache, vc);
        }
        
        // Start the vertex struct
        sprintf(strbuff, "static Vtx vtx_%s[] = {\n", cmesh->name);
        list_append(&list_output, newstring(strbuff));
        
        // Cycle through each vertex cache block and dump all the vert data
        vcnode = vcache.head;
        while (vcnode != NULL)
        {
            vcblockvertcount = 0;
            vertCache* vc = (vertCache*)vcnode->data;
            listNode* curvert = vc->verts.head;
            
            // Cycle through all the vertices so that they can be dumped
            while (curvert != NULL)
            {
                s64Face* f;
                s64Vert* vert = (s64Vert*)curvert->data;
                int texturew = 0, textureh = 0;
                n64Texture* tex = NULL;
                Vector3D normorcol;
                
                // Get the texture size
                f = find_facefromvertindex(&vc->faces, vertindex - vc->offset);
                if (f != NULL)
                    tex = f->texture;
                if (tex != NULL && tex->type == TYPE_TEXTURE)
                {
                    texturew = (tex->data).image.w;
                    textureh = (tex->data).image.h;
                }

                // Pick vertex colors or vertex normals
                if (global_vertexcolors)
                    normorcol = vector_scale(vert->color, 255);
                else
                    normorcol = vector_scale(vert->normal, 127);

                // Store the vertex string
                sprintf(strbuff, "    {%d, %d, %d, 0, %d, %d, %d, %d, %d, 255}, /* %d */\n", 
                    (int)round(vert->pos.x), (int)round(vert->pos.y), (int)round(vert->pos.z),
                    float_to_s10p5(vert->UV.x*texturew), float_to_s10p5(vert->UV.y*textureh),
                    (int)round(normorcol.x), (int)round(normorcol.y), (int)round(normorcol.z),
                        vertindex++
                );
                list_append(&list_output, newstring(strbuff));
                
                // Go to the next vertex
                curvert = curvert->next;
            }
            
            // Go to the next vertex cache block
            vcnode = vcnode->next;
        }
        list_append(&list_output, "};\n\n");
                
        // Next, the display list itself
        sprintf(strbuff, "static Gfx gfx_%s[] = {\n", cmesh->name);
        list_append(&list_output, newstring(strbuff));
        vcnode = vcache.head;
        
        while (vcnode != NULL)
        {
            vertCache* vc = (vertCache*)vcnode->data;
            listNode* curface = vc->faces.head;
            
            // Load vertices to the vertex cache
            sprintf(strbuff, "    gsSPVertex(vtx_%s+%d, %d, 0),\n", cmesh->name, vc->offset, vc->verts.size + vc->reuse);
            list_append(&list_output, newstring(strbuff));
            
            // Cycle through all the faces in this block
            while (curface != NULL)
            {
                s64Face* face = (s64Face*)curface->data;
                n64Texture* tex = face->texture;
                
                // If we have a new texture, perform a texture load
                // TODO, optimize the mesh order to reduce texture loads
                if (tex != lastTexture && tex != NULL)
                {
                    switch (tex->type)
                    {
                        case TYPE_TEXTURE:
                            if (lastTexture == NULL || lastTexture->type != TYPE_TEXTURE)
                                list_append(&list_output, "    gsDPSetCombineMode(G_CC_MODULATEIDECALA, G_CC_MODULATEIDECALA),\n");
                            sprintf(strbuff, "    gsDPLoadTextureBlock(%s, G_IM_FMT_RGBA, G_IM_SIZ_16b, %d, %d, 0, G_TX_CLAMP, G_TX_CLAMP, %d, %d, G_TX_NOLOD, G_TX_NOLOD),\n    gsDPPipeSync(),\n",
                                tex->name, tex->data.image.w, tex->data.image.h, nearest_pow2(tex->data.image.w), nearest_pow2(tex->data.image.h)
                            );
                            break;
                        case TYPE_PRIMCOL:
                            if (lastTexture == NULL || lastTexture->type != TYPE_PRIMCOL)
                                list_append(&list_output, "    gsDPSetCombineMode(G_CC_PRIMLITE, G_CC_PRIMLITE),\n    gsDPPipeSync(),\n");
                            sprintf(strbuff, "    gsDPSetPrimColor(0, 0, %d, %d, %d, 255),\n", tex->data.color.r, tex->data.color.g, tex->data.color.b);
                            break;
                    }
                    list_append(&list_output, newstring(strbuff));
                    lastTexture = tex;
                }
                
                // Draw the triangle
                if (face->vertcount == 3)
                    sprintf(strbuff, "    gsSP1Triangle(%d, %d, %d, 0),\n", face->verts[0], face->verts[1], face->verts[2]);
                else
                    sprintf(strbuff, "    gsSP2Triangles(%d, %d, %d, 0, %d, %d, %d, 0),\n", face->verts[0], face->verts[1], face->verts[2], 
                                                                                            face->verts[0], face->verts[2], face->verts[3]);
                list_append(&list_output, newstring(strbuff));
                
                // Go to the next face
                curface = curface->next;
            }
            
            // We can destroy the vertex and face data in the vertex cache struct since we're done with it
            list_destroy_deep(&vc->verts);
            list_destroy_deep(&vc->faces);
            
            // Go to the next vertex cache block
            vcnode = vcnode->next;
        }
        list_append(&list_output, "    gsSPEndDisplayList(),\n};\n\n");
         
        // Free the memory used by our vertex cache list
        list_destroy_deep(&vcache);
        
        // Go to the next mesh
        curmesh = curmesh->next;
    }
    if (!global_quiet) printf("*Finish making display lists\n");
}