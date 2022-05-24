/***************************************************************
                          optimizer.c

Performs a bunch of optimizations on the model for export and 
generates vertex caches. The optimizations are as follows:
- Sorts the meshes to reduce texture loading using a custom
  algorithm.
- Merges verticies which both use PRIMCOLOR textures
- Optimizes the triangle loading order using Forsyth, heavily 
  basing my code off the implementation by Martin Strosjo, 
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
    
#define DEBUG 1

#define FORSYTH_SCORE_SCALING       7281
#define FORSYTH_CACHE_DECAY_POWER   1.5
#define FORSYTH_LAST_TRI_SCORE      0.75
#define FORSYTH_VALENCE_BOOST_SCALE 2.0
#define FORSYTH_VALENCE_BOOST_POWER 0.5


/*********************************
             Structs
*********************************/

typedef struct {
    int         id;       // The ID of this node
    linkedList  textures; // A list of texture names used by this node
    linkedList* meshes;   // A list of meshes used by this node
    linkedList  edges;    // A list with a tuple of nodes and the corresponding edge weight
    linkedList  ignore;   // A list of nodes to ignore if this node is visited
} TSPNode;


/*********************************
             Globals
*********************************/

// Factorial lookup table, up to 12 factorial
static unsigned int factorial[] = {1, 1, 2, 6, 24, 120, 720, 5040, 40320, 362880, 3628800, 39916800, 479001600};

// Forsyth globals
static int* forsyth_posscore = NULL;
static int* forsyth_valencescore = NULL;


/*==============================
    find_index_vert_vcache
    Returns the index of a vert in a vertex cache block
    @param The vertex cache to search
    @param The vertex to find the index of
    @returns The vertex index, or -1
==============================*/

static int find_index_vert_vcache(vertCache* vcache, s64Vert* vert)
{
    int index = 0;
    
    // Iterate through the vertex list
    for (listNode* vertnode = vcache->verts.head; vertnode != NULL; vertnode = vertnode->next)
    {
        if ((s64Vert*)vertnode->data == vert)
            return index;
        index++;
    }
    
    // No vert found, return -1
    return -1;   
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
    Calculates the heuristic for a vertex
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
    @param The mesh we're optimizing
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
    listNode* curnode;
    linkedList* newvcachelist = NULL;
    bool neednewblock;
    vertCache* vcachenew;
    
    // Allocate memory for the vertex indices list
    tricount = vcacheoriginal->faces.size;
    indices = (int*) calloc(1, sizeof(int)*tricount*3);
    outindices = (int*) calloc(1, sizeof(int)*tricount*3); // To be removed later
    if (indices == NULL)
        terminate("Error: Unable to allocate memory for vertex indices list\n");
    
    // Allocate memory for the vertex triangle count list
    activetricount = (int*)calloc(1, sizeof(int)*vertcount);
    if (activetricount == NULL)
        terminate("Error: Unable to allocate memory for vertex triangle count\n");
    
    // Generate a list of vertex triangle indices
    for (curnode = vcacheoriginal->faces.head; curnode != NULL; curnode = curnode->next)
    {
        s64Face* face = (s64Face*) curnode->data;
        
        for (i=0; i<3; i++)
            indices[curtri*3+i] = find_index_vert_vcache(vcacheoriginal, face->verts[i]);
        curtri++;
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

    // Now that we have an optimized vert list, lets generate the new optimal cache block
    
    // Start by allocating memory for the new vertex cache block list
    newvcachelist = (linkedList*)calloc(1, sizeof(linkedList));
    neednewblock = TRUE;
    if (newvcachelist == NULL)
        terminate("Error: Failure allocating memory for optimized vertex cache list\n");
    
    // Now generate the blocks
    for (i=0; i<tricount*3; i+=3)
    {
        s64Face* face;
        bool addme[6];
        int newvertcount = 0;
        memset(addme, FALSE, 6);
        
        // If we need a new vertex cache block, allocate memory for it
        if (neednewblock)
        {
            vcachenew = (vertCache*)calloc(1, sizeof(vertCache));
            list_append(newvcachelist, vcachenew);
            neednewblock = FALSE;
        }
        
        // Count how many new verts we have in this face
        face = (s64Face*)list_node_from_index(&vcacheoriginal->faces, outtris[i/3])->data;
        for (j=0; j<MAXVERTS; j++)
        {
            if (!list_hasvalue(&vcachenew->verts, face->verts[j]))
            {
                addme[j] = TRUE;
                newvertcount++;
            }
        }
        
        // If the number of new verts exceed the vertex cache size, restart this loop and allocate a new block
        if (vcachenew->verts.size + newvertcount > global_cachesize)
        {
            i -= 3;
            neednewblock = TRUE;
            continue;
        }
        
        // Add all the new verts and the face
        for (j=0; j<MAXVERTS; j++)
            if (addme[j])
                list_append(&vcachenew->verts, face->verts[j]);
        list_append(&vcachenew->faces, face);
    }
           
    // Free all the memory used by the algorithm
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
    permute
    Generates permutations of the textures array and 
    stores them in a separate array
    @param The textures array
    @param The starting index 
    @param The ending index
    @param A pointer to the current index in the storage array
    @param The storage array
    @param The number of textures
==============================*/

static void permute(char** textures, int start, int end, int* count, char*** store, int texcount)
{
    if (start == end)
    {
        for (int i=0; i<texcount; i++)
            store[*count][i] = textures[i];
        (*count)++;
        return;
    }
        
    for (int i=start; i<=end; i++)
    {
        char* left = textures[start];
        char* right = textures[i];
        textures[start] = right;
        textures[i] = left;

        permute(textures, start+1, end, count, store, texcount);

        textures[start] = left;
        textures[i] = right;
    }
} 


/*==============================
    shares_textures
    Checks if two meshes share the same textures
    @param   The mesh to check the textures of
    @param   The mesh to compare to
    @returns Whether or not the two meshes share textures
==============================*/

static inline bool shares_textures(s64Mesh* a, s64Mesh* b)
{
    // If the number of textures doesn't match between meshes, then it obviously doesn't share textures
    if (a->textures.size != b->textures.size)
        return FALSE;
    
    // Go through all the textures in mesh 'a'
    for (listNode* tex_a = a->textures.head; tex_a != NULL; tex_a = tex_a->next)
    {
        bool found = FALSE;
        
        // Compare the names of the textures in 'a' with the textures in 'b'
        for (listNode* tex_b = b->textures.head; tex_b != NULL; tex_b = tex_b->next)
        {
            if (!strcmp(((n64Texture*)tex_a->data)->name, ((n64Texture*)tex_b->data)->name))
            {
                found = TRUE;
                break;
            }
        }
        
        // We didn't find a match, the two meshes don't share textures
        if (!found)
            return FALSE;
    }
    
    // Success
    return TRUE;
}


/*==============================
    optimize_textureloads
    Optimizes the texture loading order in the model
==============================*/

static void optimize_textureloads()
{
    /*
    * We want to sort meshes to reduce the amount of texture loads
    * The algorithm works as follows:
    * - Treat each mesh as a node. For a mesh that loads N textures, you must create N! nodes
    * - For instance, a mesh that loads texture A and B must have nodes A+B and B+A
    * - When all nodes are generated, connect every node to each other. If a texture swap occurs, then that edge has weight 1, 0 otherwise.
    * - Once the network has been created, solve the problem using Traveling Salesman
    * Note: If, for instance, the path A+B is taken, B+A must be ignored as it's no longer needed
    */
    
    int id = 0;
    int nodecount = 0;
    int shortestsize = 0x7FFFFFFF;
    int* shortest;
    int finalpathsize = 0;
    TSPNode* nodeslist;
    linkedList meshes_groupedby_mat = EMPTY_LINKEDLIST;
    linkedList neworder = EMPTY_LINKEDLIST;
    if (!global_quiet) printf("    Optimizing texture loading order\n");
    
    // First, group all meshes by the textures they use
    for (listNode* mesh = list_meshes.head; mesh != NULL; mesh = mesh->next)
    {
        bool found = FALSE;
                        
        // Find meshes that share textures with us
        for (listNode* e = meshes_groupedby_mat.head; e != NULL; e = e->next)
        {
            s64Mesh* meshcompare = (s64Mesh*)((linkedList*)e->data)->head->data; // Only need to compare first mesh in the list since they all share the same textures
            if (shares_textures((s64Mesh*)mesh->data, meshcompare))
            {
                list_append((linkedList*)e->data, (s64Mesh*)mesh->data);
                found = TRUE;
                break;
            }
        }
        
        // There were no meshes that had this collection of textures, so create a new list
        if (!found)
        {
            linkedList* l = (linkedList*)calloc(1, sizeof(linkedList));
            list_append(l, (s64Mesh*)mesh->data);
            list_append(&meshes_groupedby_mat, l);
            continue;
        }
    }
    
    // Now that we have a list of meshes, sorted by the textures they load, create N factorial nodes for N texture loads
    // Count how many nodes we must create
    for (listNode* e = meshes_groupedby_mat.head; e != NULL; e = e->next)
    {
        int loadfirstcount = 0;
        s64Mesh* mesh = (s64Mesh*)((linkedList*)e->data)->head->data;
        
        // Check for nodes which need to be loaded first, since we don't need to sort them
        for (listNode* texes = mesh->textures.head; texes != NULL; texes = texes->next)
            if (((n64Texture*)texes->data)->loadfirst)
                loadfirstcount++;
                
        // Ensure we don't have two or more textures with LOADFIRST in this mesh
        if (loadfirstcount > 1)
            terminate("Error: Mesh uses two textures with LOADFIRST flag\n");
            
        // Add to our total node count the factorial of the texture count (excluding textures with LOADFIRST)
        if (has_property(mesh, "NoSort"))
        {
            nodecount += 1;
            printf("    Skipping texture optimization on %s\n", mesh->name);
        }
        else
        {
            nodecount += factorial[mesh->textures.size-loadfirstcount];
        
            // Warn for too many textures
            if (mesh->textures.size > 7)
                printf("    WARNING: %s has %d textures. Consider using the 'NoSort' property.\n", mesh->name, mesh->textures.size);
        }
    }
    
    // Allocate memory for an array of Traveling Salesman nodes
    nodeslist = (TSPNode*) calloc(nodecount, sizeof(TSPNode));
    shortest = (int*) calloc(nodecount, sizeof(int));
    
    // Now setup each node
    for (listNode* e = meshes_groupedby_mat.head; e != NULL; e = e->next)
    {
        int i=0;
        int startid = id;
        char* loadfirst = NULL;
        s64Mesh* mesh = (s64Mesh*)((linkedList*)e->data)->head->data;
        bool sorttextures = !has_property(mesh, "NoSort");
        int texcount = sorttextures ? mesh->textures.size : 1;
        char** textures = (char**) calloc(mesh->textures.size, sizeof(char*));
        char*** perms = (char***) calloc(factorial[texcount], sizeof(char**));
        for (i=0 ; i<factorial[texcount]; i++)
            perms[i] = (char**) calloc(mesh->textures.size, sizeof(char*));
        
        // Initialize the textures array
        i=0;
        for (listNode* tex = mesh->textures.head; tex != NULL; tex = tex->next)
            textures[i++] = ((n64Texture*)tex->data)->name;
        
        // Generate permutations of the textures array
        i=0;
        if (sorttextures)
        {
            permute(textures, 0, texcount-1, &i, perms, texcount);
        }
        else
        {
            for (int j=0; j<mesh->textures.size; j++)
                perms[i][j] = textures[j];
        }
        
        // Get which texture needs to be loaded first
        for (listNode* tex = mesh->textures.head; tex != NULL; tex = tex->next)
        {
            if (((n64Texture*)tex->data)->loadfirst)
            {
                loadfirst = ((n64Texture*)tex->data)->name;
                break;
            }
        }
        
        // Create all the nodes
        for (i=0; i<factorial[texcount]; i++)
        {
            int end = startid+factorial[texcount];
            
            // Skip if the first texture in this permutation is not the one that needs to be loaded first
            if (loadfirst != NULL && strcmp(loadfirst, perms[i][0]) != 0)
                continue;
            
            // Initialize the node
            nodeslist[id].id = id;
            for (int j=0; j<mesh->textures.size; j++)
                list_append(&nodeslist[id].textures, perms[i][j]);
            nodeslist[id].meshes = (linkedList*)e->data;
            
            // If we load a texture first, then we have less nodes to ignore
            if (loadfirst != NULL)
                end = startid+factorial[texcount-1];
            
            // Generate the ignore list
            for (int j=startid; j<end; j++)
            {
                if (j != id)
                {
                    #pragma GCC diagnostic push
                    #pragma GCC diagnostic ignored "-Wint-to-pointer-cast"
                    list_append(&nodeslist[id].ignore, (int*)j); // It's not a pointer, I'm lying
                    #pragma GCC diagnostic pop
                }
            }
            id++;
        }
        
        // Free the memory used to generate the permutations
        for (i=0; i<factorial[texcount]; i++)
            free(perms[i]);
        free(perms);
        free(textures);
    }
    
    // Generate the undirected weighted graph
    for (int i=0; i<nodecount; i++)
    {
        for (int j=0; j<nodecount; j++)
        {
            if (nodeslist[i].id != nodeslist[j].id)
            {
                // Check we're not in the ignore list
                #pragma GCC diagnostic push
                #pragma GCC diagnostic ignored "-Wint-to-pointer-cast"
                if (list_hasvalue(&nodeslist[i].ignore, (int*)nodeslist[j].id))
                    continue;
                #pragma GCC diagnostic pop
                
                // Create a tuple
                Tuple* edge = (Tuple*) malloc(sizeof(Tuple));
                #pragma GCC diagnostic push
                #pragma GCC diagnostic ignored "-Wint-to-pointer-cast"
                edge->a = (int*)nodeslist[j].id; // It's not a pointer, I'm lying

                // If the exit material is different from the enter material, then create a touple with weight 1
                if (strcmp((char*)nodeslist[i].textures.tail->data, (char*)nodeslist[j].textures.tail->data) != 0)
                    edge->b = (int*)1; // It's not a pointer, I'm lying
                else
                    edge->b = (int*)0; // It's not a pointer, I'm lying
                #pragma GCC diagnostic pop
                list_append(&nodeslist[i].edges, edge);
            }
        }
    }
    
    // Solve the traveling salesman problem
    // Using nearest neighbor because I'm lazy
    // TODO: Use a better algorithm to make this even more optimized
    for (int i=0; i<nodecount; i++)
    {
        int pathindex = 0;
        int leftindex = 0;
        int pathsize = 0;
        TSPNode* prev = NULL;
        TSPNode* cur = &nodeslist[i];
        int* path = (int*) calloc(nodecount, sizeof(int));
        linkedList left = EMPTY_LINKEDLIST;
        path[pathindex++] = cur->id;
        
        // Initialize the list of nodes to visit 
        for (int j=0; j<nodecount; j++)
        {
            if (cur->id != nodeslist[j].id)
            {
                #pragma GCC diagnostic push
                #pragma GCC diagnostic ignored "-Wint-to-pointer-cast"
                if (list_hasvalue(&cur->ignore, (int*)nodeslist[j].id))
                    continue;
                list_append(&left, (int*)nodeslist[j].id);
                #pragma GCC diagnostic pop
            }
        }
        
        // Do the algorithm
        while (left.size > 0)
        {
            int nextsize = 0x7FFFFFFF;
            TSPNode* next = NULL;
            
            // Find the shortest path
            for (listNode* e = cur->edges.head; e != NULL; e = e->next)
            {
                int extra = 0;
                bool inlist = FALSE;
                Tuple* t = (Tuple*)e->data;
                
                // Check if this node's ID is in our list of nodes left to visit
                for (listNode* l = left.head; l != NULL; l = l->next)
                {
                    #pragma GCC diagnostic push
                    #pragma GCC diagnostic ignored "-Wpointer-to-int-cast"
                    if ((int)l->data == (int)t->a)
                    {
                        inlist = TRUE;
                        break;
                    }
                    #pragma GCC diagnostic pop
                }
                if (!inlist)
                    continue;
                    
                // Calculate extra weight due to previous texture switch
                #pragma GCC diagnostic push
                #pragma GCC diagnostic ignored "-Wpointer-to-int-cast"
                if (prev != NULL && strcmp((char*)nodeslist[(int)t->a].textures.head->data, (char*)prev->textures.tail->data) != 0)
                    extra = 1;
                
                // Check if this path is the shortest
                if ((int)t->b+extra < nextsize)
                {
                    next = &nodeslist[(int)t->a];
                    nextsize = (int)t->b+extra;
                }
                #pragma GCC diagnostic pop
            }
            
            // Travel down that path
            path[pathindex++] = next->id;
            #pragma GCC diagnostic push
            #pragma GCC diagnostic ignored "-Wint-to-pointer-cast"
            #pragma GCC diagnostic ignored "-Wpointer-to-int-cast"
            free(list_remove(&left, (int*)next->id));
            
            // Remove all nodes that we should ignore from the list of nodes left to visit
            for (listNode* e = next->ignore.head; e != NULL; e = e->next)
            {
                for (listNode* l = left.head; l != NULL; l = l->next)
                {
                    if ((int)l->data == (int)e->data)
                    {
                        free(list_remove(&left, (int*)e->data));
                        break;
                    }
                }
            }
            #pragma GCC diagnostic pop
            prev = cur;
            cur = next;
            pathsize += nextsize;
        }
        
        // If this solution was better, store the result
        if (pathsize < shortestsize)
        {
            finalpathsize = pathindex;
            memset(shortest, 0, nodecount);
            memcpy(shortest, path, finalpathsize*sizeof(int));
            shortestsize = pathsize;
        }
        
        // Cleanup
        list_destroy(&left);
        free(path);
        break;
    }
    
    // Print the optimal order
    #if DEBUG
        printf("Optimal loading order:\n");
        for (int i=0; i<finalpathsize; i++)
        {
            printf("%d (%d)\n", shortest[i], finalpathsize);
            for (listNode* m = nodeslist[shortest[i]].meshes->head; m != NULL; m = m->next)
            {
                printf("%16s loads ", ((s64Mesh*)m->data)->name);
                for (listNode* t = nodeslist[shortest[i]].textures.head; t != NULL; t = t->next)
                    printf("%s, ", (char*)t->data);
                printf("\n");
            }
        }
    #endif  
    
    // Now we need to sort the faces inside each mesh to fit the new loading order
    for (int i=0; i<finalpathsize; i++)
    {
        for (listNode* m = nodeslist[shortest[i]].meshes->head; m != NULL; m = m->next)
        {
            s64Mesh* mesh = (s64Mesh*)m->data;
            linkedList faces_by_tex = EMPTY_LINKEDLIST;
            for (listNode* f = mesh->faces.head; f != NULL; f = f->next)
                for (listNode* t = nodeslist[shortest[i]].textures.head; t != NULL; t = t->next)
                    if (!strcmp(((s64Face*)f->data)->texture->name, t->data))
                        list_append(&faces_by_tex, f->data);
            list_destroy(&mesh->faces);
            list_combine(&mesh->faces, &faces_by_tex);
        }
    }

    // Make the global mesh list use the new mesh order
    for (int i=0; i<finalpathsize; i++)
        for (listNode* m = nodeslist[shortest[i]].meshes->head; m != NULL; m = m->next)
            list_append(&neworder,  m->data);
    list_destroy(&list_meshes);
    list_meshes = neworder;
    
    // Finally, sort the textures list in each mesh, since it's not in the new order
    for (listNode* m = list_meshes.head; m != NULL; m = m->next)
    {
        s64Mesh* mesh = (s64Mesh*)m->data;
        n64Texture* lasttex = NULL;
        linkedList newtexorder = EMPTY_LINKEDLIST;
        for (listNode* f = mesh->faces.head; f != NULL; f = f->next)
        {
            s64Face* face = (s64Face*)f->data;
            if (face->texture != lasttex)
            {
                list_append(&newtexorder, face->texture);
                lasttex = face->texture;
            }
        }
        list_destroy(&mesh->textures);
        mesh->textures = newtexorder;
    }
    
    // Free the memory used by our algorithm
    for (int i=0; i<nodecount; i++)
    {
        list_destroy_deep(&nodeslist[i].edges);
        list_destroy(&nodeslist[i].textures);
        list_destroy(&nodeslist[i].ignore);
    }
    free(shortest);
    free(nodeslist);
    for (listNode* e = meshes_groupedby_mat.head; e != NULL; e = e->next)
        list_destroy((linkedList*)e->data);
    list_destroy(&meshes_groupedby_mat);
}


/*==============================
    optimize_duplicatedverts
    Optimizes duplicated vertices using PRIMCOLOR
==============================*/

static void optimize_duplicatedverts()
{
    int merged = 0;
    if (!global_quiet) printf("    Merging unecessary vertices\n");
    
    for (listNode* meshnode = list_meshes.head; meshnode != NULL; meshnode = meshnode->next)
    {
        s64Mesh* mesh = (s64Mesh*)meshnode->data;
        for (listNode* vertnode1 = mesh->verts.head; vertnode1 != NULL; vertnode1 = vertnode1->next)
        {
            s64Vert* vert1 = (s64Vert*) vertnode1->data;
            listNode* prevvert2;
            bool v1_onlyprimcolor = TRUE;
            
            // Check this vertex is only used by primcolor faces
            for (listNode* facenode = mesh->faces.head; facenode != NULL; facenode = facenode->next)
            {
                s64Face* face = (s64Face*)facenode->data;
                for (int i=0; i<MAXVERTS; i++)
                {
                    if (face->verts[i] == vert1)
                    {
                        if (face->texture->type != TYPE_PRIMCOL)
                        {
                            v1_onlyprimcolor = FALSE;
                            break;
                        }
                    }
                }
                if (!v1_onlyprimcolor)
                    break;
            }
            if (!v1_onlyprimcolor)
                continue;
            
            // Look through all other vertices
            prevvert2 = mesh->verts.head;
            for (listNode* vertnode2 = mesh->verts.head; vertnode2 != NULL; vertnode2 = vertnode2->next)
            {
                if (vertnode1 != vertnode2)
                {
                    bool v2_onlyprimcolor = TRUE;
                    s64Vert* vert2 = (s64Vert*) vertnode2->data;
                    
                    // Check this vertex is only used by primcolor faces
                    for (listNode* facenode = mesh->faces.head; facenode != NULL; facenode = facenode->next)
                    {
                        s64Face* face = (s64Face*)facenode->data;
                        for (int i=0; i<MAXVERTS; i++)
                        {
                            if (face->verts[i] == vert2)
                            {
                                if (face->texture->type != TYPE_PRIMCOL)
                                {
                                    v2_onlyprimcolor = FALSE;
                                    break;
                                }
                            }
                        }
                        if (!v2_onlyprimcolor)
                            break;
                    }
                    if (!v2_onlyprimcolor)
                    {
                        prevvert2 = vertnode2;
                        continue;
                    }
                    
                    // If everything matches (except UV's, since they don't matter), merge this vertex
                    if ((vert1->pos.x == vert2->pos.x && vert1->pos.y == vert2->pos.y && vert1->pos.z == vert2->pos.z) && 
                        (vert1->normal.x == vert2->normal.x && vert1->normal.y == vert2->normal.y && vert1->normal.z == vert2->normal.z) && 
                        (vert1->color.x == vert2->color.x && vert1->color.y == vert2->color.y && vert1->color.z == vert2->color.z))
                    {
                        free(list_remove(&mesh->verts, vert2));
                        merged++;
                        
                        // Loop through all faces and correct the indices
                        for (listNode* facenode = mesh->faces.head; facenode != NULL; facenode = facenode->next)
                        {
                            s64Face* face = (s64Face*)facenode->data;
                            for (int i=0; i<MAXVERTS; i++)
                                if (face->verts[i] == vert2)
                                    face->verts[i] = vert1;
                        }
                        free(vert2);
                        vertnode2 = prevvert2;
                    }
                }
                prevvert2 = vertnode2;
            }
        }
    }
    
    if (!global_quiet) printf("        %d verts merged\n", merged);
}


/*==============================
    split_verts_by_texture
    Takes in a mesh and splits the vertices and triangles by texture
    into separate vertex cache structs
    @param The mesh to parse
==============================*/

 static void split_verts_by_texture(s64Mesh* mesh)
{
    // Go through each texture
    for (listNode* texnode = mesh->textures.head; texnode != NULL; texnode = texnode->next)
    {
        n64Texture* tex = (n64Texture*) texnode->data;
        vertCache* vcache = (vertCache*) calloc(1, sizeof(vertCache));
        
        // Go through each face in the mesh
        for (listNode* facenode = mesh->faces.head; facenode != NULL; facenode = facenode->next)
        {
            s64Face* face = (s64Face*) facenode->data;
            
            // If this face uses the texture we're searching for
            if (face->texture == tex)
            {
                // Append it to the vertex cache face list
                list_append(&vcache->faces, face);
                
                // Go through each vert in this face
                for (int i=0; i<MAXVERTS; i++)
                {
                    s64Vert* vert = face->verts[i];
                    
                    // Ensure this vert isn't already in the cache block
                    if (list_hasvalue(&vcache->verts, vert))
                        continue;
                        
                    // If it isn't, add it
                    list_append(&vcache->verts, vert);
                }
            }
        }
        
        // Add this vertex cache block to the mesh's cache list
        list_append(&mesh->vertcache, vcache);
    }
}


/*==============================
    combine_caches
    Attempts to combine vertex caches
    Ineffient since it's O(N^2)
    @param The mesh to parse
==============================*/

static void combine_caches(s64Mesh* mesh)
{
    linkedList removedlist = EMPTY_LINKEDLIST;
    
    // Iterate through all the nodes to check which can be combined
    for (listNode* firstnode = mesh->vertcache.head; firstnode != NULL; firstnode = firstnode->next)
    {
        vertCache* vc1 = (vertCache*)firstnode->data;
            
        // Skip if this node is marked for deletion
        if (list_hasvalue(&removedlist, vc1))
            continue;
        
        // Now look through all the other elements
        for (listNode* secondnode = mesh->vertcache.head; secondnode != NULL; secondnode = secondnode->next)
        {
            vertCache* vc2 = (vertCache*)secondnode->data;
            
            // Skip if this node is marked for deletion
            if (list_hasvalue(&removedlist, vc2))
                continue;
            
            // If we're not looking at the same vert cache block
            if (vc1 != vc2)
            {
                // If these two together would fit in the chace, then combine them
                if (vc1->verts.size + vc2->verts.size <= global_cachesize)
                {
                    list_combine(&vc1->verts, &vc2->verts);
                    list_combine(&vc1->faces, &vc2->faces); 
                    list_append(&removedlist, vc2);
                }
            }
        }
    }
    
    // Remove the redundant caches from the mesh's cache list
    for (listNode* vcachenode = removedlist.head; vcachenode != NULL; vcachenode = vcachenode->next)
        list_remove(&mesh->vertcache, vcachenode->data);
    
    // Garbage collect
    list_destroy_deep(&removedlist);
}


/*==============================
    optimize_mdl
    Performs all sorts of optimizations on the model
==============================*/

void optimize_mdl()
{
    if (!global_quiet) printf("Optimizing model\n");
    
    // Initialize Forsyth, we might need it
    forsyth_init();
    
    // First, lets try to optimize the texture loading order in the entire model
    if (list_meshes.size > 1 && list_textures.size > 1)
        optimize_textureloads();
    
    // If there's two duplicated vertices with same normals and vcolors, but they're both used for primitive color textures, we can safely merge them (since UV's are useless)
    optimize_duplicatedverts();
    
    // Now that our model is all nice and optimized, go through each model
    for (listNode* meshnode = list_meshes.head; meshnode != NULL; meshnode = meshnode->next)
    {
        s64Mesh* mesh = (s64Mesh*)meshnode->data;
        
        // See if the model fits in the vertex cache
        if (mesh->verts.size > global_cachesize)
        {
            int index = 0;
            printf("    Mesh '%s' too large for vertex cache, splitting by texture.\n", mesh->name);
        
            // Oh dear, this model doesn't fit... Let's split the mesh by texture and see if that helps
            split_verts_by_texture(mesh);
            
            // Try to combine any cache blocks that could fit together after having been split by texture
            combine_caches(mesh);
            
            // If that didn't help, then split the vertex block further and duplicate verts with the help of Forsyth
            for (listNode* vcachenode = mesh->vertcache.head; vcachenode != NULL; vcachenode = vcachenode->next)
            {
                vertCache* vcache = (vertCache*)vcachenode->data;
                if (vcache->verts.size > global_cachesize)
                {
                    linkedList* list;
                    printf("        Cache needs to be split further, applying Forsyth + duplicating verts.\n");
                    
                    // Apply Forsyth on this cache node and retrieve a new list of vertex caches to replace this one
                    list = forsyth(vcache);
                    free(list_swapindex_withlist(&mesh->vertcache, index, list));
                    vcachenode = list->tail;
                    index += list->size;
                    continue;
                }
                index++;
            }
        }
        else
        {
            // Model fits fine, lets just shove every vert into a cache.
            vertCache* vcache = (vertCache*) calloc(1, sizeof(vertCache));
            if (vcache == NULL)
                terminate("Error: Unable to allocate memory for vertex cache\n");
            vcache->verts = mesh->verts;
            vcache->faces = mesh->faces;
            list_append(&mesh->vertcache, vcache);
        }
    }
    
    // Finished
    if (!global_quiet) printf("Finished optimizing\n");
    free(forsyth_posscore);
    free(forsyth_valencescore);
}