/***************************************************************
                          optimizer.c

Performs a bunch of optimizations on the model for export and 
generates vertex caches. The optimizations are as follows:
- Sorts the meshes to reduce material loading using a custom
  algorithm.
- Merges verticies which both use PRIMCOLOR materials
- Optimizes the triangle loading order using Forsyth, heavily 
  basing my code off the implementation by Martin Strosjo, 
  available here: http://www.martin.st/thesis/forsyth.cpp
***************************************************************/

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <math.h>
#include "main.h"
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
    int         id;        // The ID of this node
    linkedList  materials; // A list of material names used by this node
    linkedList* meshes;    // A list of meshes used by this node
    linkedList  edges;     // A list with a tuple of nodes and the corresponding edge weight
    linkedList  ignore;    // A list of nodes to ignore if this node is visited
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
    Generates permutations of the materials array and 
    stores them in a separate array
    @param The materials array
    @param The starting index 
    @param The ending index
    @param A pointer to the current index in the storage array
    @param The storage array
    @param The number of materials
==============================*/

static void permute(char** materials, int start, int end, int* count, char*** store, int matcount)
{
    if (start == end)
    {
        for (int i=0; i<matcount; i++)
            store[*count][i] = materials[i];
        (*count)++;
        return;
    }
        
    for (int i=start; i<=end; i++)
    {
        char* left = materials[start];
        char* right = materials[i];
        materials[start] = right;
        materials[i] = left;

        permute(materials, start+1, end, count, store, matcount);

        materials[start] = left;
        materials[i] = right;
    }
} 


/*==============================
    shares_materials
    Checks if two meshes share the same materials
    @param   The mesh to check the materials of
    @param   The mesh to compare to
    @returns Whether or not the two meshes share materials
==============================*/

static inline bool shares_materials(s64Mesh* a, s64Mesh* b)
{
    // If the number of materials doesn't match between meshes, then it obviously doesn't share materials
    if (a->materials.size != b->materials.size)
        return FALSE;
    
    // Go through all the materials in mesh 'a'
    for (listNode* mat_a = a->materials.head; mat_a != NULL; mat_a = mat_a->next)
    {
        bool found = FALSE;
        
        // Compare the names of the materials in 'a' with the materials in 'b'
        for (listNode* mat_b = b->materials.head; mat_b != NULL; mat_b = mat_b->next)
        {
            if (!strcmp(((n64Material*)mat_a->data)->name, ((n64Material*)mat_b->data)->name))
            {
                found = TRUE;
                break;
            }
        }
        
        // We didn't find a match, the two meshes don't share materials
        if (!found)
            return FALSE;
    }
    
    // Success
    return TRUE;
}


/*==============================
    optimize_materialloads
    Optimizes the material loading order in the model
==============================*/

static void optimize_materialloads()
{
    /*
    * We want to sort meshes to reduce the amount of material loads
    * The algorithm works as follows:
    * - Treat each mesh as a node. For a mesh that loads N materials, you must create N! nodes
    * - For instance, a mesh that loads material A and B must have nodes A+B and B+A
    * - When all nodes are generated, connect every node to each other. If a material swap occurs, then that edge has weight 1, 0 otherwise.
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
    if (!global_quiet) printf("    Optimizing material loading order\n");
    
    // First, group all meshes by the materials they use
    for (listNode* mesh = list_meshes.head; mesh != NULL; mesh = mesh->next)
    {
        bool found = FALSE;
                        
        // Find meshes that share materials with us
        for (listNode* e = meshes_groupedby_mat.head; e != NULL; e = e->next)
        {
            s64Mesh* meshcompare = (s64Mesh*)((linkedList*)e->data)->head->data; // Only need to compare first mesh in the list since they all share the same materials
            if (shares_materials((s64Mesh*)mesh->data, meshcompare))
            {
                list_append((linkedList*)e->data, (s64Mesh*)mesh->data);
                found = TRUE;
                break;
            }
        }
        
        // There were no meshes that had this collection of materials, so create a new list
        if (!found)
        {
            linkedList* l = (linkedList*)calloc(1, sizeof(linkedList));
            list_append(l, (s64Mesh*)mesh->data);
            list_append(&meshes_groupedby_mat, l);
            continue;
        }
    }
    
    // Now that we have a list of meshes, sorted by the materials they load, create N factorial nodes for N materials loads
    // Count how many nodes we must create
    for (listNode* e = meshes_groupedby_mat.head; e != NULL; e = e->next)
    {
        int loadfirstcount = 0;
        s64Mesh* mesh = (s64Mesh*)((linkedList*)e->data)->head->data;
        
        // Check for nodes which need to be loaded first, since we don't need to sort them
        for (listNode* mats = mesh->materials.head; mats != NULL; mats = mats->next)
            if (((n64Material*)mats->data)->loadfirst)
                loadfirstcount++;
                
        // Ensure we don't have two or more materials with LOADFIRST in this mesh
        if (loadfirstcount > 1)
            terminate("Error: Mesh uses two materials with LOADFIRST flag\n");
            
        // Add to our total node count the factorial of the material count (excluding materials with LOADFIRST)
        if (has_property(mesh, "NoSort"))
        {
            nodecount += 1;
            printf("    Skipping material optimization on %s\n", mesh->name);
        }
        else
        {
            nodecount += factorial[mesh->materials.size-loadfirstcount];
        
            // Warn for too many materials
            if (mesh->materials.size > 7)
                printf("    WARNING: %s has %d materials. Consider using the 'NoSort' property.\n", mesh->name, mesh->materials.size);
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
        bool sortmaterials = !has_property(mesh, "NoSort");
        int matcount = sortmaterials ? mesh->materials.size : 1;
        char** materials = (char**) calloc(mesh->materials.size, sizeof(char*));
        char*** perms = (char***) calloc(factorial[matcount], sizeof(char**));
        for (i=0 ; i<factorial[matcount]; i++)
            perms[i] = (char**) calloc(mesh->materials.size, sizeof(char*));
        
        // Initialize the materials array
        i=0;
        for (listNode* mat = mesh->materials.head; mat != NULL; mat = mat->next)
            materials[i++] = ((n64Material*)mat->data)->name;
        
        // Generate permutations of the material array
        i=0;
        if (sortmaterials)
        {
            permute(materials, 0, matcount-1, &i, perms, matcount);
        }
        else
        {
            for (int j=0; j<mesh->materials.size; j++)
                perms[i][j] = materials[j];
        }
        
        // Get which material needs to be loaded first
        for (listNode* mat = mesh->materials.head; mat != NULL; mat = mat->next)
        {
            if (((n64Material*)mat->data)->loadfirst)
            {
                loadfirst = ((n64Material*)mat->data)->name;
                break;
            }
        }
        
        // Create all the nodes
        for (i=0; i<factorial[matcount]; i++)
        {
            int end = startid+factorial[matcount];
            
            // Skip if the first material in this permutation is not the one that needs to be loaded first
            if (loadfirst != NULL && strcmp(loadfirst, perms[i][0]) != 0)
                continue;
            
            // Initialize the node
            nodeslist[id].id = id;
            for (int j=0; j<mesh->materials.size; j++)
                list_append(&nodeslist[id].materials, perms[i][j]);
            nodeslist[id].meshes = (linkedList*)e->data;
            
            // If we load a material first, then we have less nodes to ignore
            if (loadfirst != NULL)
                end = startid+factorial[matcount-1];
            
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
        for (i=0; i<factorial[matcount]; i++)
            free(perms[i]);
        free(perms);
        free(materials);
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
                if (strcmp((char*)nodeslist[i].materials.tail->data, (char*)nodeslist[j].materials.tail->data) != 0)
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
    // TODO: Texture loads take up more DL commands, prefer avoiding those than primcol
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
                    
                // Calculate extra weight due to previous material switch
                #pragma GCC diagnostic push
                #pragma GCC diagnostic ignored "-Wpointer-to-int-cast"
                if (prev != NULL && strcmp((char*)nodeslist[(int)t->a].materials.head->data, (char*)prev->materials.tail->data) != 0)
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
                for (listNode* t = nodeslist[shortest[i]].materials.head; t != NULL; t = t->next)
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
            linkedList faces_by_mat = EMPTY_LINKEDLIST;
            for (listNode* f = mesh->faces.head; f != NULL; f = f->next)
                for (listNode* t = nodeslist[shortest[i]].materials.head; t != NULL; t = t->next)
                    if (!strcmp(((s64Face*)f->data)->material->name, t->data))
                        list_append(&faces_by_mat, f->data);
            list_destroy(&mesh->faces);
            list_combine(&mesh->faces, &faces_by_mat);
        }
    }

    // Make the global mesh list use the new mesh order
    for (int i=0; i<finalpathsize; i++)
        for (listNode* m = nodeslist[shortest[i]].meshes->head; m != NULL; m = m->next)
            list_append(&neworder,  m->data);
    list_destroy(&list_meshes);
    list_meshes = neworder;
    
    // Finally, sort the material list in each mesh, since it's not in the new order
    for (listNode* m = list_meshes.head; m != NULL; m = m->next)
    {
        s64Mesh* mesh = (s64Mesh*)m->data;
        n64Material* lastmat = NULL;
        linkedList newmatorder = EMPTY_LINKEDLIST;
        for (listNode* f = mesh->faces.head; f != NULL; f = f->next)
        {
            s64Face* face = (s64Face*)f->data;
            if (face->material != lastmat)
            {
                list_append(&newmatorder, face->material);
                lastmat = face->material;
            }
        }
        list_destroy(&mesh->materials);
        mesh->materials = newmatorder;
    }
    
    // Free the memory used by our algorithm
    for (int i=0; i<nodecount; i++)
    {
        list_destroy_deep(&nodeslist[i].edges);
        list_destroy(&nodeslist[i].materials);
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
                        if (face->material->type != TYPE_PRIMCOL)
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
                                if (face->material->type != TYPE_PRIMCOL)
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
    split_verts_by_material
    Takes in a mesh and splits the vertices and triangles by material
    into separate vertex cache structs
    @param The mesh to parse
==============================*/

 static void split_verts_by_material(s64Mesh* mesh)
{
    // Go through each material
    for (listNode* matnode = mesh->materials.head; matnode != NULL; matnode = matnode->next)
    {
        n64Material* mat = (n64Material*) matnode->data;
        vertCache* vcache = (vertCache*) calloc(1, sizeof(vertCache));
        
        // Go through each face in the mesh
        for (listNode* facenode = mesh->faces.head; facenode != NULL; facenode = facenode->next)
        {
            s64Face* face = (s64Face*) facenode->data;
            
            // If this face uses the material we're searching for
            if (face->material == mat)
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
    
    // First, lets try to optimize the material loading order in the entire model
    if (list_meshes.size > 1 && list_materials.size > 1)
        optimize_materialloads();
    
    // If there's two duplicated vertices with same normals and vcolors, but they're both used for primitive color materials, we can safely merge them (since UV's are useless)
    optimize_duplicatedverts();
    
    // Now that our model is all nice and optimized, go through each model
    for (listNode* meshnode = list_meshes.head; meshnode != NULL; meshnode = meshnode->next)
    {
        s64Mesh* mesh = (s64Mesh*)meshnode->data;
        
        // See if the model fits in the vertex cache
        if (mesh->verts.size > global_cachesize)
        {
            int index = 0;
            printf("    Mesh '%s' too large for vertex cache, splitting by material.\n", mesh->name);
        
            // Oh dear, this model doesn't fit... Let's split the mesh by material and see if that helps
            split_verts_by_material(mesh);
            
            // Try to combine any cache blocks that could fit together after having been split by material
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