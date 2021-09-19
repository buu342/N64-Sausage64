bl_info = {
    "name": "Sausage64 Character Export",
    "description": "Exports a sausage-link character with animations.",
    "author": "Buu342",
    "version": (1, 0),
    "blender": (2, 80, 0),
    "location": "File > Export > Sausage64 Character (.S64)",
    "warning": "",
    "wiki_url": "https://github.com/buu342/Blender-Sausage64",
    "tracker_url": "",
    "support": 'COMMUNITY',
    "category": "Import-Export"
}

# TODO:
## More vertex normals
## Correct vertex indices for when different objects share the same bone
## Correct vertex indices for when different vertices in an object share different bones
## bpy.context.scene.render.fps

import bpy
import math
import bmesh
import operator
import mathutils
import collections

DebugS64acterExport = 0

class S64Vertex:
    def __init__(self):
        self.coor = None # Vertex X Y Z
        self.norm = None # Vertex Normal X Y Z
        self.colr = None # Vertex Color R G B
        self.uv   = None # Vertex U V
    def __str__(self):
        return ("S64Vert: [%.4f, %.4f, %.4f]" % self.coor[:])

class S64Face:
    def __init__(self):
        self.verts = [] # List of vertex indices
        self.mat = ""   # Material name for this face
    def __str__(self):
        string = "S64Face:"
        for i in self.verts:
            string = string+" "+str(i)
        string = string+" "+self.mat
        return string

class S64Mesh:
    def __init__(self, name):
        self.name  = name # Skeleton name
        self.verts = {}   # Dict of vertices (We'll later convert to list)
        self.faces = []   # List of faces
        self.root  = None # Bone root location
    def __str__(self):
        string = "S64Mesh: '"+self.name+"'\n"
        string = string+"Root: "+str(self.root)+"\n"
        string = string + "Verts:\n"
        for i in self.verts:
            string = string + "\t"+str(i)+" -> "+str(self.verts[i])+"\n"
        string = string + "Faces:\n"
        for i in self.faces:
            string = string + "\t"+str(i)+"\n"
        return string
        
class S64Anim:
    def __init__(self, name):
        self.name   = name # Animation name
        self.frames = {}   # Dict of animation frames
    def __str__(self):
        string = "S64Anim: '"+self.name+"'\n"
        for i in self.frames:
            string = string + "\tFrame "+str(i)+"\n"
            for j in self.frames[i]:
                string = string+str(self.frames[i][j])+"\n"
        return string
        
class S64KeyFrame:
    def __init__(self, bone):
        self.bone  = bone # Affected bone name
        self.pos   = None # Bone position
        self.ang   = None # Bone rotation
        self.scale = None # Bone scale
    def __str__(self):
        string = "\t\tS64Keyframe: '"+self.bone+"'\n"
        string = string+"\t\t Pos = "+str(self.pos)+"\n"
        string = string+"\t\t Ang = "+str(self.ang)+"\n"
        string = string+"\t\t Scale = "+str(self.scale)
        return string

class settingsExport:
    triangulate  = None
    onlyvisible  = None
    onlyselected = None
    
def isNewBlender():
    return bpy.app.version >= (2, 80)

def matmul(a, b):
    if (isNewBlender()):
        return operator.matmul(a, b)
    return a*b

def writeFile(self, object, finalList, animList):
    with open(self.filepath, 'w') as file:
        file.write("/**********************************\n")
        file.write("      Sausage64 Character Mesh\n")
        file.write("         Script by Buu342\n")
        file.write("            Version 1.0\n")
        file.write("**********************************/\n\n")
        
        # Write the mesh data
        for n, m in finalList.items():
        
            # Start a new mesh
            file.write("BEGIN MESH "+n+"\n")
            file.write("ROOT "+("%.4f " % m.root.x)+("%.4f " % m.root.y)+("%.4f\n" % m.root.z))
            
            # Write the list of vertices
            file.write("BEGIN VERTICES\n")
            for k, v in m.verts.items():
                file.write("%.4f %.4f %.4f " % v.coor[:])
                file.write("%.4f %.4f %.4f " % v.norm[:])
                file.write(("%.4f " % v.colr[0])+("%.4f " % v.colr[1])+("%.4f " % v.colr[2]))
                file.write("%.4f %.4f" % v.uv[:])
                file.write("\n")
            file.write("END VERTICES\n")
            
            # Write the list of faces
            file.write("BEGIN FACES\n")
            for f in m.faces:
                file.write(str(len(f.verts))+" ")
                for v in f.verts:
                    file.write(str(v)+" ")
                if (f.mat != "" and f.mat is not None):
                    file.write(f.mat+"\n")
                else:
                    file.write("None\n")
            file.write("END FACES\n")
            
            # End this mesh
            file.write("END MESH "+n+"\n\n")
            
        # Write the animation data
        for n, a in animList.items():
        
            # Start a new Animation
            file.write("BEGIN ANIMATION "+n+"\n")
            
            # Write the list of keyframes
            for kf in a.frames:
                file.write("BEGIN KEYFRAME "+str(int(kf))+"\n")
                for b in a.frames[kf]:
                    frame = a.frames[kf][b]
                    file.write(frame.bone)
                    file.write((" %.4f " % frame.pos.x)+("%.4f " % frame.pos.y)+("%.4f" % frame.pos.z))
                    file.write((" %.4f " % frame.ang.x)+("%.4f " % frame.ang.y)+("%.4f" % frame.ang.z))
                    file.write((" %.4f " % frame.scale.x)+("%.4f " % frame.scale.y)+("%.4f\n" % frame.scale.z))
                file.write("END KEYFRAME "+str(int(kf))+"\n")
                
            file.write("END ANIMATION "+n+"\n\n")
            
    self.report({'INFO'}, 'File exported sucessfully!')
    return {'FINISHED'}

def addFace(finalList, f, name, vertex_normals, vertex_colors, vertex_uvs, materials_list):
    face = S64Face()
    for vi in f.verts:
        face.verts.append(vi.index)
        vert = S64Vertex()
        vert.coor = vi.co[:]
        vert.norm = vertex_normals[vi.index][:]
        try:
            vert.colr = vertex_colors[vi.index][:]
        except KeyError:
            vert.colr = (1.0, 1.0, 1.0)
        vert.uv = vertex_uvs[vi.index][:]
        finalList[name].verts[vi.index] = vert
    try:
        face.mat = materials_list[f.material_index].name
    except IndexError:
        face.mat = "None"
    finalList[name].faces.append(face)

def setupData(self, object, skeletonList, meshList, settingsList):
    finalList = {}
    animList = {}
    
    # The first element should always be None (for objects without bones)
    finalList["None"] = S64Mesh("None")
    
    # Go through the list of bones and add all deformable bones
    originPoses = {}
    for v in skeletonList:
    
        # Get the original pose and then change the armature to rest pose
        originPoses[v] = v.data.pose_position
        v.data.pose_position = "REST"
        if (isNewBlender()):
            bpy.context.view_layer.update()
        else:
            bpy.context.scene.update()
        
        # Go through all the bones
        for b in v.data.bones:
            if b.use_deform: # Ignore non deformable bones
                if (b.name == "None"):
                    self.report({'ERROR'}, 'You should not have a bone named "None"')
                    return 'CANCELLED'
                finalList[b.name] = S64Mesh(b.name)
                finalList[b.name].root = mathutils.Vector((b.head_local.x, b.head_local.y, b.head_local.z))
    
    # Now go through all the meshes and find out which bone they belong to
    for m in meshList:
    
        # If auto smooth is enabled, calculate the split normals
        if (m.data.has_custom_normals):
            m.data.calc_normals_split()
        
        # Create a copy of the mesh
        mcopy = None
        if (isNewBlender()):
            mcopy = m.evaluated_get(bpy.context.evaluated_depsgraph_get()).to_mesh()
        else:
            mcopy = m.to_mesh(scene=bpy.context.scene, apply_modifiers=True, settings='PREVIEW', calc_undeformed=True)
        
        # Setup a bmesh and validate the data
        bm = bmesh.new()
        bm.from_mesh(mcopy)
        if (settingsList.triangulate):
            bmesh.ops.triangulate(bm, faces=bm.faces[:], quad_method=0, ngon_method=0)
        bm.verts.index_update()
        bm.verts.ensure_lookup_table()
        bm.edges.ensure_lookup_table()
        bm.faces.ensure_lookup_table()
        if (isNewBlender()):
            tris = bm.calc_loop_triangles()
        else:
            tris = bm.calc_tessface()
            
        # Get the vertex normals and create a dictionary that maps their normals to their indices
        vertex_normals = {}
        if (m.data.has_custom_normals):
            for l in m.data.loops:
                vertex_normals[l.vertex_index] = l.normal
        else:
            for f in bm.faces:
                for l in f.loops:
                    for vi in f.verts:
                        vertex_normals[vi.index] = vi.normal

        # Get the vertex colors and create a dictionary that maps their colors to their indices
        vertex_colors = {}
        for name, cl in bm.loops.layers.color.items():
            for i, tri in enumerate(tris):
                for loop in tri:
                    vertex_colors[loop.vert.index] = loop[cl][:]
                    
        # Get the vertex UVs and create a dictionary that maps their UVs to their indices
        vertex_uvs = {}
        for f in bm.faces:
            for l in f.loops:
                try:
                    vertex_uvs[l.vert.index] = mathutils.Vector((l[bm.loops.layers.uv.active].uv[0], 1-l[bm.loops.layers.uv.active].uv[1]))
                except AttributeError:
                    vertex_uvs[l.vert.index] = (0.0, 0.0)
                    
        # Get a list of materials used by this mesh
        materials_list = m.material_slots
        for mat in materials_list:
            if (mat.name == "None"):
                self.report({'ERROR'}, 'You should not have a material named "None"')
                return 'CANCELLED'
        
        # Get the vertex groups and create a dictionary that maps their name to their indices
        layer_deform = bm.verts.layers.deform.active
        vgroup_names = {vgroup.index: vgroup.name for vgroup in m.vertex_groups}
        
        # Append all faces to their respective vertex group data in finalList
        if (layer_deform is not None):
            for f in bm.faces:
            
                # Error if a face has too many verts
                if (len(f.verts) > 4):
                    self.report({'ERROR'}, 'Faces should have more than 4 vertices!')
                    return 'CANCELLED'
            
                # Get the vertex groups of the first vertex of this face
                v = f.verts[0][layer_deform].items()
                for g in v:
                
                    # If the vertex has weights, add all verts in this face to the list of vertices and faces in this mesh
                    if (g[1] > 0.5): 
                        try:
                            addFace(finalList, f, vgroup_names[g[0]], vertex_normals, vertex_colors, vertex_uvs, materials_list)
                        except KeyError:
                            self.report({'ERROR'}, 'Vertex group "'+vgroup_names[g[0]]+'" does not match any bone names.')
                            return 'CANCELLED'
                            
                        # Move onto the next face
                        break
        else:
            for f in bm.faces:
                addFace(finalList, f, "None", vertex_normals, vertex_colors, vertex_uvs, materials_list) # These faces/vertices have no weights

    # Remove any list element that's empty
    for i in list(finalList.keys()):
        if (len(finalList[i].faces) == 0):
            del finalList[i]
        
    # Fix the bone poses
    for v in skeletonList:
        v.data.pose_position = originPoses[v]
        if (isNewBlender()):
            bpy.context.view_layer.update()
        else:
            bpy.context.scene.update()
        
    # Now iterate through all the animations and add them to the list
    if (len(bpy.data.actions) > 0):
        for a in bpy.data.actions:
        
            # Ignore actions with fake users
            if (a.use_fake_user):
                continue
                
            anim = S64Anim(a.name)
            start = a.frame_range.x
            end = a.frame_range.y
            
            # Cycle through all the fcurves and add keyframe numbers that exist
            for fcurve in a.fcurves:
                for p in fcurve.keyframe_points:
                    if not p.co.x in anim.frames:
                        anim.frames[p.co.x] = {}
           
            # Get the current frame so that we can reset it later
            framebefore = bpy.context.scene.frame_current
           
            # Now that we have a list of places where keyframes exist, lets get anim data for that frame
            for k in anim.frames:

                # Modify the current Blender keyframe so we can get the pose data
                bpy.context.scene.frame_set(k)

                # Go through all skeletons and add the bone's data
                for s in skeletonList:
                
                    # Get the action before we mess with it, and then mess with it
                    actionbefore = s.animation_data.action
                    s.animation_data.action = a
                    if (isNewBlender()):
                        bpy.context.view_layer.update()
                    else:
                        bpy.context.scene.update()
                    
                    for b in s.data.bones:
                    
                        # Ensure this bone exists in finalList
                        if (not b.name in finalList):
                            continue
                        
                        boneframe = S64KeyFrame(b.name)
                        
                        # Grab the bone data for its rest and posed version
                        obone = bpy.data.objects['Armature'].data.bones[b.name]
                        pbone = bpy.data.objects['Armature'].pose.bones[b.name]

                        # Get the translation matricies from the head location
                        trans = mathutils.Matrix.Translation(obone.head_local)
                        itrans = mathutils.Matrix.Translation(-obone.head_local)

                        # Calculate the bone space matricies and convert them to world space
                        mat_final = matmul(pbone.matrix, obone.matrix_local.inverted())
                        mat_final = matmul(itrans, matmul(mat_final, trans))
                        
                        # Store the different data from the matricies
                        boneframe.pos   = mat_final.to_translation()
                        boneframe.ang   = mat_final.to_euler()
                        boneframe.scale = mat_final.to_scale()
                        
                        # Add this bone's frame data to the to the list of frame data for this keyframe
                        anim.frames[k][b.name] = boneframe
                
                    # Put the armature back to its original animation (IE the one before we started exporting)
                    s.animation_data.action = actionbefore
                
            # Fix the frame number
            bpy.context.scene.frame_set(framebefore)
            if (isNewBlender()):
                bpy.context.view_layer.update()
            else:
                bpy.context.scene.update()
                
            # Store the animation in the animation list
            animList[anim.name] = anim
    
    # Sort the faces by texture
    for i in finalList:
        finalList[i].faces.sort(key=lambda x: x.mat, reverse=False)    
    
    # Because the vertex indices are not necessarily in order, let's fix that
    for i in finalList:
        vertdict = {}
        vertcount = 0
        ordered = collections.OrderedDict()
        
        # Fix meshes that have no root (because they don't have bones)
        if (finalList[i].root == None):
            finalList[i].root = mathutils.Vector((0.0, 0.0, 0.0))
            
        # Get the list of verts in order of appearance from the faces
        for f in finalList[i].faces:
            for k, v in enumerate(f.verts):
                if (not f.verts[k] in vertdict):
                    vertdict[f.verts[k]] = vertcount
                    vertcount = vertcount + 1
                f.verts[k] = vertdict[f.verts[k]]
        
        # Then sort the vertex dictionary based on the lookup table made in the previous loop
        for v in finalList[i].verts:
            ordered[vertdict[v]] = finalList[i].verts[v]
        finalList[i].verts = collections.OrderedDict(sorted(ordered.items()))
        
    # Animation keyframes are also not in order, so sort them too
    for i in animList:
        animList[i].frames = collections.OrderedDict(sorted(animList[i].frames.items()))
    
    # Print the model data for debugging purposes
    if (DebugS64acterExport):
        for i in finalList:
            print(finalList[i])
        for i in animList:
            print(animList[i])
        
    # Return the list with all the meshes and animations
    return finalList, animList

class ObjectExport(bpy.types.Operator):
    """Exports a sausage-link character with animations."""
    bl_idname = "object.export_sausage64"
    bl_label = "Sausage64 Character Export" # The text on the export button
    bl_options = {'REGISTER', 'UNDO'}
    filename_ext = ".S64"
    
    filter_glob         = bpy.props.StringProperty(default="*.S64", options={'HIDDEN'}, maxlen=255)
    setting_triangulate = bpy.props.BoolProperty(name="Triangulate", description="Triangulate objects", default=False)
    setting_selected    = bpy.props.BoolProperty(name="Selected only", description="Export selected objects only", default=False)
    setting_visible     = bpy.props.BoolProperty(name="Visible only", description="Export visible objects only", default=True)
    filepath            = bpy.props.StringProperty(subtype='FILE_PATH')    
    
    def execute(self, context):
        skeletonList = []
        meshList = []
        settingsList = settingsExport()
        self.filepath = bpy.path.ensure_ext(self.filepath, ".S64")
                
        # First, store the settings in an object
        settingsList.triangulate  = self.setting_triangulate
        settingsList.onlyselected = self.setting_selected
        settingsList.onlyvisible  = self.setting_visible
        
        # Pick out what objects we're going to look over
        list = bpy.data.objects
        if (settingsList.onlyselected):
            list = bpy.context.selected_objects
            
        # Start the actual parsing by organizing all the objects in the scene into an array
        for v in list:
            if (not settingsList.onlyvisible or ((isNewBlender() and v.visible_get()) or (not isNewBlender() and v.is_visible(bpy.context.scene)))):
                if v.type == 'ARMATURE':
                    skeletonList.append(v)
                elif v.type == 'MESH':
                    meshList.append(v)
                
        # Next, organize the data further by splitting them into categories
        finalList, animList = setupData(self, context, skeletonList, meshList, settingsList)
        if (finalList == 'CANCELLED'):
            return {'CANCELLED'}
            
        # Finally, dump all the organized data to a file
        writeFile(self, context, finalList, animList);
        return {'FINISHED'}

    def invoke(self, context, event):
        if not self.filepath:
            fname = bpy.path.display_name_from_filepath(bpy.data.filepath)
            if (fname == ""):
                fname = "Untitled"
            self.filepath = bpy.path.ensure_ext(fname, ".S64")
        context.window_manager.fileselect_add(self)
        return {'RUNNING_MODAL'}

# Blender Script registration
def menu_func_export(self, context):
    self.layout.operator(ObjectExport.bl_idname, text="Sausage64 Character (.S64)")

def register():
    bpy.utils.register_class(ObjectExport)
    if (isNewBlender()):
        bpy.types.TOPBAR_MT_file_export.append(menu_func_export)
    else:
        bpy.types.INFO_MT_file_export.append(menu_func_export)

def unregister():
    bpy.utils.unregister_class(ObjectExport)
    if (isNewBlender()):
        bpy.types.TOPBAR_MT_file_export.remove(menu_func_export)
    else:
        bpy.types.INFO_MT_file_export.remove(menu_func_export)

if __name__ == "__main__":
    register()