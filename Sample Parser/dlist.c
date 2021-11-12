/***************************************************************
                            dlist.c
                             
Constructs a display list string for outputting later.
To optimize the triangle list, I utilize the Forsyth algorithm,
heavily basing my code off the implementation by Martin Strosjo, 
available here: http://www.martin.st/thesis/forsyth.cpp
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

static void split_verts_by_texture(s64Mesh* mesh, linkedList* vcachelist)
{
    int i, offset = 0;
    bool* addedverts;
    listNode* facenode;
    n64Texture* curtex = NULL;
    vertCache* curvcache = NULL;
    
    // Allocate memory for our list of added verts
    addedverts = (bool*) calloc(1, sizeof(bool)*mesh->verts.size);
    if (addedverts == NULL)
        terminate("Error: Unable to allocate memory for added verts list\n");
    
    // Go through all the faces so that we can see what textures they're using
    for (facenode = mesh->faces.head; facenode != NULL; facenode = facenode->next)
    {
        s64Face* face = (s64Face*)facenode->data;
        
        // If we found a new texture, create a new vertex cache
        if (curtex == NULL || (face->texture != curtex && face->texture->type != TYPE_OMIT))
        {
            vertCache* vcache;
            curtex = face->texture;
                
            // Calculate the new vertex list offset
            if (curvcache != NULL)
                offset += curvcache->verts.size;
            
            // Create a new vertex cache block and append it to our vertex cache list
            vcache = (vertCache*) calloc(1, sizeof(vertCache));
            if (vcache == NULL)
                terminate("Error: Unable to allocate memory for vertex cache\n");
            vcache->offset = offset;
            list_append(vcachelist, vcache);
            curvcache = vcache;
        }
        
        // Add this vert to the current vertex cache block's list of verts, if it's not there yet
        for (i=0; i<face->vertcount; i++)
        {
            int vindex = face->verts[i];
            if (!addedverts[vindex])
            {
                s64Vert* vert = (s64Vert*) list_node_from_index(&mesh->verts, vindex)->data;
                addedverts[vindex] = TRUE;
                list_append(&curvcache->verts, vert);
            }
        }
        
        // Add this face to the vertex cache block's list of faces, then check the next face
        list_append(&curvcache->faces, face);
    }
    
    // Clean up memory we've used
    free(addedverts);
}


/*==============================
    offset_vertindices
    Fixes the vertex index offset in a list of faces so they start at the given offset
    @param The list of faces
    @param The offset starting value
==============================*/

static void offset_vertindices(linkedList* faces, int offset)
{
    int vertcount = 0;
    listNode* facenode;
    hashTable vertstable;
    memset(&vertstable, 0, sizeof(vertstable));
    
    for (facenode = faces->head; facenode != NULL; facenode = facenode->next)
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
                #pragma GCC diagnostic push
                #pragma GCC diagnostic ignored "-Wint-to-pointer-cast"
                hkeyval = htable_append(&vertstable, face->verts[i], (void*)(offset+vertcount)); // It's not a pointer, I'm lying
                #pragma GCC diagnostic pop
                vertcount++;
            }
            
            // Fix the vert index from the hashtable value
            #pragma GCC diagnostic push
            #pragma GCC diagnostic ignored "-Wpointer-to-int-cast"
            face->verts[i] = (int)hkeyval->value;
            #pragma GCC diagnostic pop
        }
    }
    
    // Clean up memory we've used
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
    listNode* firstnode, *secondnode;
    linkedList removedlist = EMPTY_LINKEDLIST;
    
    // Iterate through all the nodes to check which can be combined
    for (firstnode = vcachelist->head; firstnode != NULL; firstnode = firstnode->next)
    {
        vertCache* vc1 = (vertCache*)firstnode->data;
        
        // Now look through all the other elements (double for loop style)
        for (secondnode = vcachelist->head; secondnode != NULL; secondnode = secondnode->next)
        {
            vertCache* vc2 = (vertCache*)secondnode->data;
            
            // If we're not looking at the same vert cache block
            if (vc1 != vc2)
            {
                // If these two together would fit in the chace, then combine them
                if (vc1->verts.size + vc2->verts.size <= global_cachesize)
                {
                    // Fix vertex offsets in the new vertex list and then combine the lists
                    offset_vertindices(&vc2->faces, vc1->verts.size);
                    list_combine(&vc1->verts, &vc2->verts);
                    list_combine(&vc1->faces, &vc2->faces); 
                    list_append(&removedlist, list_remove(vcachelist, vc2));
                }
            }
        }
    }
    
    // Recalculate all the offsets (needs to be done anyway since split_verts_by_texture() doesn't fix the indices)
    for (firstnode = vcachelist->head; firstnode != NULL; firstnode = firstnode->next)
    {
        vertCache* vcfix = (vertCache*)firstnode->data;
        vcfix->offset = offset;
        offset += vcfix->verts.size;
        offset_vertindices(&vcfix->faces, 0);
    }
    
    // Garbage collect unused vertex cache blocks (since they were merged)
    list_destroy_deep(&removedlist);
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
            terminate("Unable to allocate memory for Forsyth cache pos score\n");
    }
    
    // Allocate memory for the valence score array
    if (forsyth_valencescore == NULL)
    {
        forsyth_valencescore = (int*)malloc(sizeof(int)*global_cachesize);
        if (forsyth_valencescore == NULL)
            terminate("Unable to allocate memory for Forsyth valence score\n");
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
    Calculates the heuristic for a  vertex
    @param The amount of triangles shared by this vertex
    @param The cache position of this vertex
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
    Applies the forsyth algorithm for organizing vertices to prevent cache 
    misses.
    https://tomforsyth1000.github.io/papers/fast_vert_cache_opt.html
    After it generates the optimal list, this function also then generates a
    linked list with a set of new cache blocks to replace the old (too big) one
    @param The vertex cache to optimize
    @returns A list of vertex caches, split to fit the cache limit
==============================*/

static linkedList* forsyth(vertCache* vcacheoriginal)
{
    int i, j;
    int *indices, *activetricount;
    int curtri = 0, tricount = 0, sum = 0, outpos = 0, scanpos = 0, besttri = -1, bestscore = -1;
    int vertcount = vcacheoriginal->verts.size;
    int* offsets, *lastscore, *cachetag, *triscore, *triindices, *outtris, *outindices, *tempcache;
    bool* triadded;
    int largestvertindex = 0, lowestvertindex = 0, vertindexcount = 0, lastcachesplit = 0;
    hashTable vertstable;
    linkedList newvertorder = EMPTY_LINKEDLIST;
    vertCache* curvcache = NULL;
    n64Texture* meshtex;
    listNode* curnode;
    linkedList* newvcachelist;
    
    // State we're using Forsyth
    printf("Applying Forsyth to cache block\n");
    
    // Calculate the number of triangles (can't just do faces->size due to quads)
    for (curnode = vcacheoriginal->faces.head; curnode != NULL; curnode = curnode->next)
    {
        s64Face* face = (s64Face*) curnode->data;
        tricount++;
        if (face->vertcount == 4)
            tricount++;
    }
    
    // Allocate memory for the vertex indices list
    indices = (int*) calloc(1, sizeof(int)*tricount*3);
    outindices = (int*) calloc(1, sizeof(int)*tricount*3); // To be removed later
    if (indices == NULL)
        terminate("Error: Unable to allocate memory for vertex indices list\n");
    
    // Allocate memory for the vertex triangle count list
    activetricount = (int *)calloc(1, sizeof(int)*vertcount);
    if (activetricount == NULL)
        terminate("Error: Unable to allocate memory for vertex triangle count\n");
    
    // Generate a list of vertex triangle indices
    for (curnode = vcacheoriginal->faces.head; curnode != NULL; curnode = curnode->next)
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
        
    // Now that we have an optimized list of verts, lets create a linked list with the verts in the new order (and fix the indices, while we're at it)
    j = 0;
    memset(&vertstable, 0, sizeof(vertstable));
    for (i=0; i<tricount*3; i++)
    {
        dictNode* hkeyval = htable_getkey(&vertstable, outindices[i]);
    
        // If this vertex index isn't in our hashtable yet, add it, and to our new vertex list
        if (hkeyval == NULL)
        {
            #pragma GCC diagnostic push
            #pragma GCC diagnostic ignored "-Wint-to-pointer-cast"
            hkeyval = htable_append(&vertstable, outindices[i], (void*)(j++)); // It's not a pointer, I'm lying
            #pragma GCC diagnostic pop
            list_append(&newvertorder, (s64Vert*)list_node_from_index(&vcacheoriginal->verts, outindices[i])->data);
        }
        
        // Fix the vert index from the hashtable value
        #pragma GCC diagnostic push
        #pragma GCC diagnostic ignored "-Wpointer-to-int-cast"
        outindices[i] = (int) hkeyval->value;
        #pragma GCC diagnostic pop
    }
    htable_destroy(&vertstable);
    
    // Allocate memory for our new vertex cache list
    newvcachelist = (linkedList*) calloc(1, sizeof(linkedList));
    if (newvcachelist == NULL)
        terminate("Error: Unable to allocate memory for new vertex cache list\n");
        
    // Now iterate through the optimized list again, start generating triangles and adding them to our cache list
    meshtex = ((s64Face*)(vcacheoriginal->faces.head)->data)->texture; // Since this mesh is already split by texture, we can rely on the first face's texture applying to all faces
    for (i=0; i<tricount*3; i+=3)
    {
        s64Face* face;
        int oldlargest = largestvertindex, oldlowest = lowestvertindex;
    
        // If we don't have a current vertex cache, create a new one and add it to our list
        if (curvcache == NULL)
        {
            curvcache = (vertCache*) calloc(1, sizeof(vertCache));
            if (curvcache == NULL)
                terminate("Error: Unable to allocate memory for corrected vertex cache\n");
            list_append(newvcachelist, curvcache);
        }
        
        // Update the largest and lowest vertex index tracker
        for (j=i; j<i+3; j++)
        {
            if (largestvertindex < outindices[j])
                largestvertindex = outindices[j];
            else if (lowestvertindex > outindices[j])
                lowestvertindex = outindices[j];
        }
        
        // If adding a vertex from this face exceeds our cache size, restart this loop so we create a new cache and try this face again
        if ((largestvertindex-lowestvertindex) >= global_cachesize)
        {
            if (lastcachesplit == oldlargest+1) // We have an infinite loop!
                terminate("Error: Unable to split cache block with requested size. Try separating some faces?\n");
            lastcachesplit = oldlargest+1;
            lowestvertindex = 0x7FFFFFFF;
            curvcache = NULL;
            i -= 3;
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
        
        // Update the offset and reuse values
        curvcache->offset = lastcachesplit;
        curvcache->reuse = curvcache->offset-lowestvertindex;
    }
    
    // Now that we have good cache blocks with the faces added in, last thing we need to do is add the correct vertex indices and offsets, one last time
    for (curnode = newvcachelist->head; curnode != NULL; curnode = curnode->next)
    {
        listNode* facenode;
        vertCache* vcache = (vertCache*)curnode->data;
        
        // Fix the offset by its reuse value
        vcache->offset -= vcache->reuse;
        
        // Iterate through the faces
        for (facenode = vcache->faces.head; facenode != NULL; facenode = facenode->next)
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
                    
                    // Add to the vert list if this vert was not in the previous cache block
                    if (face->verts[i]-vcache->offset >= vcache->reuse)
                        list_append(&vcache->verts, vert);
                }

                // Correct the vertex index to start from zero
                face->verts[i] -= vcache->offset;
            }
        }
        
        // Fix the offset from the original cache's values so that they're what we actually expect
        vcache->offset += vcacheoriginal->offset;
        htable_destroy(&vertstable);
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
    @param The list of vertex cache blocks
==============================*/

static void check_splitverts(linkedList* vcachelist)
{
    int cachelistindex = 0;
    listNode* curnode;
    
    // Iterate through all the cache blocks
    for (curnode = vcachelist->head; curnode != NULL; curnode = curnode->next)
    {
        vertCache* vc = (vertCache*) curnode->data;
        
        // If this cache block is too big
        if (vc->verts.size > global_cachesize)
        {
            // Use Forsyth to generate an optimal vert loading order + generate a list of new split cache blocks
            linkedList* newvclist = forsyth(vc);

            // Remove the old vertex cache block from the list and append our new list in its place
            free(list_swapindex_withlist(vcachelist, cachelistindex, newvclist));
            free(newvclist);
            
            // Fix the next node as the pointers have changed
            curnode = newvclist->tail;
            cachelistindex += newvclist->size;
        }
        cachelistindex++;
    }
}


/*==============================
    find_texture_fromvertindex
    Returns the texture from a face that has the given vertex index in a list of vertex caches
    @param The list of vertex caches
    @param The vertex index
    @returns The texture assigned to the FIRST face with the given vertex index, or NULL if none is found
==============================*/

static n64Texture* find_texture_fromvertindex(linkedList* vcachelist, int index)
{
    listNode* vcnode;
    
    // Iterate through the vertex caches
    for (vcnode = vcachelist->head; vcnode != NULL; vcnode = vcnode->next)
    {
        int i;
        listNode* facenode;
        vertCache* vcache = (vertCache*)vcnode->data;
        
        // Iterate through the face list
        for (facenode = vcache->faces.head; facenode != NULL; facenode = facenode->next)
        {
            s64Face* face = (s64Face*)facenode->data;
            
            // Check if the index is in this face
            for (i=0; i<face->vertcount; i++)
                if (face->verts[i]+vcache->offset == index)
                    return face->texture;
        }
    }
    
    // No face found, return NULL
    return NULL;
}


/*==============================
    construct_dl
    Constructs a display list and stores it
    in a temporary file
==============================*/

void construct_dl()
{
    FILE* fp;
    listNode* meshnode;
    char strbuff[STRBUF_SIZE];
    n64Texture* lastTexture = NULL;
    char ismultimesh = (list_meshes.size > 1);
    
    // Precalculate Forsyth tables as we might need them
    forsyth_init();
    
    // Open a temp file to write our display list to
    sprintf(strbuff, "temp_%s", global_outputname);
    fp = fopen(strbuff, "w+");
    if (fp == NULL)
        terminate("Error: Unable to open temporary file for writing\n");
        
    // Begin display list construction
    if (!global_quiet) printf("*Constructing display lists\n");
    
    // Vertex data header
    fprintf(fp, "\n// Custom combine mode to allow mixing primitive and vertex colors\n"
                "#ifndef G_CC_PRIMLITE\n    #define G_CC_PRIMLITE SHADE,0,PRIMITIVE,0,0,0,0,PRIMITIVE\n#endif\n\n\n"
                "/*********************************\n"
                "              Models\n"
                "*********************************/\n\n"
    );
    
    // Iterate through all the meshes
    for (meshnode = list_meshes.head; meshnode != NULL; meshnode = meshnode->next)
    {
        s64Mesh* mesh = (s64Mesh*)meshnode->data;
        listNode* vcachenode;
        linkedList vcachelist = EMPTY_LINKEDLIST;
        int vertindex = 0;
        
        // If the model will not fit in our vertex cache, we must use split it
        if (mesh->verts.size > global_cachesize)
        {
            if (!global_quiet) printf("Optimizing '%s' for vertex cache size\n", mesh->name);
            
            // First, split everything by texture and see if that improves our situation
            split_verts_by_texture(mesh, &vcachelist);
            
            // Try to combine any cache blocks that could fit together
            combine_caches(&vcachelist);
            
            // If that didn't help, then split the vertex block further with the help of Forsyth
            check_splitverts(&vcachelist);
            
            // TODO: If that still didn't help (because we had an impossible to split model), then duplicate problem vertices and try Forsyth again
        }
        else
        {
            // Otherwise, just feed this block the vertex and face list directly from the mesh
            vertCache* vcache = (vertCache*) calloc(1, sizeof(vertCache));
            if (vcache == NULL)
                terminate("Error: Unable to allocate memory for vertex cache\n");
            vcache->verts = mesh->verts;
            vcache->faces = mesh->faces;
            list_append(&vcachelist, vcache);
        }
        
        // Cycle through the vertex cache list and dump the vertices
        fprintf(fp, "static Vtx vtx_%s", global_modelname);
        if (ismultimesh)
            fprintf(fp, "_%s[] = {\n", mesh->name);
        fprintf(fp, "[] = {\n");
        for (vcachenode = vcachelist.head; vcachenode != NULL; vcachenode = vcachenode->next)
        {
            vertCache* vcache = (vertCache*)vcachenode->data;
            listNode* vertnode;
            
            // Cycle through all the verts
            for (vertnode = vcache->verts.head; vertnode != NULL; vertnode = vertnode->next)
            {
                int texturew = 0, textureh = 0;
                s64Vert* vert = (s64Vert*)vertnode->data;
                n64Texture* tex = find_texture_fromvertindex(&vcachelist, vertindex);
                Vector3D normorcol = {0, 0, 0};
                
                // Ensure the texture is valid
                if (tex == NULL)
                    terminate("Error: Inconsistent face/vertex texture information\n");
                
                // Retrieve texture/normal/color data for this vertex
                switch (tex->type)
                {
                    case TYPE_TEXTURE:
                        // Get the texture size
                        texturew = (tex->data).image.w;
                        textureh = (tex->data).image.h;
                        
                        // Intentional fallthrough
                    case TYPE_PRIMCOL:
                        // Pick vertex normals or vertex colors, depending on the texture flag
                        if (tex_hasgeoflag(tex, "G_LIGHTING"))
                            normorcol = vector_scale(vert->normal, 127);
                        else
                            normorcol = vector_scale(vert->color, 255);
                        break;
                    case TYPE_OMIT:
                        break;
                }
                
                // Dump the vert data
                fprintf(fp, "    {%d, %d, %d, 0, %d, %d, %d, %d, %d, 255}, /* %d */\n", 
                    (int)round(vert->pos.x), (int)round(vert->pos.y), (int)round(vert->pos.z),
                    float_to_s10p5(vert->UV.x*texturew), float_to_s10p5(vert->UV.y*textureh),
                    (int)round(normorcol.x), (int)round(normorcol.y), (int)round(normorcol.z),
                        vertindex++
                );
            }
        }
        fprintf(fp, "};\n\n");
        
        // If a texture swap didn't happen, then combine two single faces into a single 2triangles call.
        
        // Then cycle through the vertex cache list again, but now dump the display list
        fprintf(fp, "static Gfx gfx_%s", global_modelname);
        if (ismultimesh)
            fprintf(fp, "_%s[] = {\n", mesh->name);
        fprintf(fp, "[] = {\n");
        for (vcachenode = vcachelist.head; vcachenode != NULL; vcachenode = vcachenode->next)
        {
            listNode* facenode;
            vertCache* vcache = (vertCache*)vcachenode->data;
            
            // Load a new vertex block
            fprintf(fp, "    gsSPVertex(vtx_%s", global_modelname);
            if (ismultimesh)
                fprintf(fp, "_%s", mesh->name);
            fprintf(fp, "+%d, %d, 0),\n", vcache->offset, vcache->verts.size + vcache->reuse);
            
            // Cycle through all the faces
            for (facenode = vcache->faces.head; facenode != NULL; facenode = facenode->next)
            {
                s64Face* face = (s64Face*)facenode->data;
                n64Texture* tex = face->texture;
                
                // If we want to skip the initial display list setup, then change the value of our last texture to skip the next if statement
                if (lastTexture == NULL && !global_initialload)
                    lastTexture = tex;
            
                // If a texture change was detected, load the new texture data
                if (lastTexture != tex && tex->type != TYPE_OMIT)
                {
                    int i;
                    bool pipesync = FALSE;
                    bool changedgeo = FALSE;
                    
                    // Check for different cycle type
                    if (lastTexture == NULL || strcmp(tex->cycle, lastTexture->cycle) != 0)
                    {
                        fprintf(fp, "    gsDPSetCycleType(%s),\n", tex->cycle);
                        pipesync = TRUE;
                    }
                    
                    // Check for different render mode
                    if (lastTexture == NULL || strcmp(tex->rendermode1, lastTexture->rendermode1) != 0 || strcmp(tex->rendermode2, lastTexture->rendermode2) != 0)
                    {
                        fprintf(fp, "    gsDPSetRenderMode(%s, %s),\n", tex->rendermode1, tex->rendermode2);
                        pipesync = TRUE;
                    }
                    
                    // Check for different combine mode
                    if (lastTexture == NULL || strcmp(tex->combinemode1, lastTexture->combinemode1) != 0 || strcmp(tex->combinemode2, lastTexture->combinemode2) != 0)
                    {
                        fprintf(fp, "    gsDPSetCombineMode(%s, %s),\n", tex->combinemode1, tex->combinemode2);
                        pipesync = TRUE;
                    }
                    
                    // Check for different texture filter
                    if (lastTexture == NULL || strcmp(tex->texfilter, lastTexture->texfilter) != 0)
                    {
                        fprintf(fp, "    gsDPSetTextureFilter(%s),\n", tex->texfilter);
                        pipesync = TRUE;
                    }
                    
                    // Check for different geometry mode
                    if (lastTexture != NULL)
                    {
                        int flagcountold = 0, flagcountnew = 0;
                        for (i=0; i<MAXGEOFLAGS; i++)
                        {
                            int j;
                            bool hasthisflag = FALSE;
                            
                            // Skip empty flags
                            if (tex->geomode[i][0] == '\0')
                                continue;
                                
                            flagcountnew++;
                            flagcountold = 0;
                                
                            // Look through all the old texture's flags
                            for (j=0; j<MAXGEOFLAGS; j++)
                            {
                                if (lastTexture->geomode[j][0] == '\0')
                                    continue;
                                flagcountold++;
                                if (!strcmp(tex->geomode[i], lastTexture->geomode[j]))
                                {
                                    hasthisflag = TRUE;
                                    break;
                                }
                            }
                            
                            // If a flagchange was detected, the display list must be updated
                            if (!hasthisflag)
                            {
                                changedgeo = TRUE;
                                break;
                            }
                        }
                        
                        // If the number of flags changed, then we need to update the display list
                        if (flagcountold != flagcountnew)
                            changedgeo = TRUE;
                    }
                    else
                        changedgeo = TRUE;
                        
                    // If a geometry mode flag changed, then update the display list
                    if (changedgeo)
                    {
                        bool appendline = FALSE;
                    
                        // TODO: Smartly omit display list commands based on what changed
                        fprintf(fp, "    gsSPClearGeometryMode(0xFFFFFFFF),\n");
                        fprintf(fp, "    gsSPSetGeometryMode(");
                        for (i=0; i<MAXGEOFLAGS; i++)
                        {
                            if (tex->geomode[i][0] == '\0')
                                continue;
                            if (appendline)
                            {
                                fprintf(fp, " | ");
                                appendline = FALSE;
                            }
                            fprintf(fp, "%s", tex->geomode[i]);
                            appendline = TRUE;
                        }
                        fprintf(fp, "),\n");
                    }
                    
                    // Load the texture if it wasn't marked as DONTLOAD
                    if (!tex->dontload)
                    {
                        if (tex->type == TYPE_TEXTURE)
                        {
                            fprintf(fp, "    gsDPLoadTextureBlock(%s, %s, %s, %d, %d, 0, %s, %s, %d, %d, G_TX_NOLOD, G_TX_NOLOD),\n",
                                tex->name, tex->data.image.coltype, tex->data.image.colsize, tex->data.image.w, tex->data.image.h, 
                                tex->data.image.texmodes, tex->data.image.texmodet, nearest_pow2(tex->data.image.w), nearest_pow2(tex->data.image.h)
                            );
                            pipesync = TRUE;
                        }
                        else if (tex->type == TYPE_PRIMCOL)
                            fprintf(fp, "    gsDPSetPrimColor(0, 0, %d, %d, %d, 255),\n", tex->data.color.r, tex->data.color.g, tex->data.color.b);
                    }
                    
                    // Call a pipesync if needed
                    if (pipesync)
                        fprintf(fp, "    gsDPPipeSync(),\n");

                    // Update the last texture
                    lastTexture = tex;
                }
                
                // Dump the face data
                if (face->vertcount == 3)
                    fprintf(fp, "    gsSP1Triangle(%d, %d, %d, 0),\n", face->verts[0], face->verts[1], face->verts[2]);
                else
                    fprintf(fp, "    gsSP2Triangles(%d, %d, %d, 0, %d, %d, %d, 0),\n", face->verts[0], face->verts[1], face->verts[2], 
                                                                                       face->verts[0], face->verts[2], face->verts[3]);
            }
            
            // Newline if we have another vertex block to load
            if (vcachenode->next != NULL)
                fprintf(fp, "\n");
        }
        fprintf(fp, "    gsSPEndDisplayList(),\n};\n\n");
        
        // Free the memory we used for dumping this mesh
        for (vcachenode = vcachelist.head; vcachenode != NULL; vcachenode = vcachenode->next)
        {
            vertCache* vcache = (vertCache*)vcachenode->data;
            list_destroy(&vcache->verts);
            list_destroy(&vcache->faces);    
        }
        list_destroy_deep(&vcachelist);
    }
    
    // Close the temporary file and cleanup
    fclose(fp);
    free(forsyth_posscore);
    free(forsyth_valencescore);
    
    // State we finished
    if (!global_quiet) printf("*Finish building display lists\n");
}