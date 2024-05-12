bl_info = {
    "name": "Sausage64 Character Import",
    "description": "Imports a sausage-link character with animations.",
    "author": "Buu342",
    "version": (1, 0),
    "blender": (2, 80, 0),
    "location": "File > Import > Sausage64 Character (.S64)",
    "warning": "",
    "wiki_url": "https://github.com/buu342/Blender-Sausage64",
    "tracker_url": "",
    "support": 'COMMUNITY',
    "category": "Import-Export"
}

import os
import bpy
import bmesh
import random
import mathutils
import traceback

DebugS64Export = False

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
        self.name   = name # Skeleton name
        self.verts  = []   # List of vertices
        self.faces  = []   # List of faces
        self.mats   = []   # List of materials used by this mesh
        self.props  = []   # List of custom properties
        self.root   = None # Bone root location
        self.parent = None # Parent mesh
    def __str__(self):
        string = "S64Mesh: '"+self.name+"'\n"
        string = string+"Root: "+str(self.root)+"\n"
        string = string+"Materials: "+str(self.mats)+"\n"
        string = string + "Verts:\n"
        for k, v in enumerate(self.verts):
            string = string + "\t"+str(k)+" -> "+str(v)+"\n"
        string = string + "Faces:\n"
        for i in self.faces:
            string = string + "\t"+str(i)+"\n"
        return string
    def sharesMats(self, other):
        if (len(self.mats) != len(other.mats)):
           return False
        for m in self.mats:
            if (not m in other.mats):
                return False
        return True

class S64Anim:
    def __init__(self, name):
        self.name   = name # Animation name
        self.frames = {}   # Dict of animation frames
    def __str__(self):
        string = "S64Anim: '"+self.name+"'\n"
        for i in self.frames:
            string = string + "\tFrame "+str(i)+"\n"
            for j in self.frames[i]:
                string = string+str(j)+"\n"
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

def isNewBlender():
    return bpy.app.version >= (2, 80)

def ParseS64(path):
    LEXSTATE_NONE = 0
    LEXSTATE_COMMENTBLOCK = 1
    LEXSTATE_MESH = 2
    LEXSTATE_VERTICES = 3
    LEXSTATE_FACES = 4
    LEXSTATE_ANIMATION = 5
    LEXSTATE_KEYFRAME = 6

    meshes = []
    anims = []
    materials = []

    curmesh = -1
    curvert = -1
    prevface = -1
    curface = -1
    curanim = -1
    curkeyframe = -1
    curframedata = -1
    curmat = -1

    state = [LEXSTATE_NONE]
    with open(path, "r") as fs:
        for l in fs:
            line = l.split()
            lineit = iter(line)
            while True:
                try:
                    chunk = next(lineit).rstrip()

                    if ("//" in chunk):
                        break

                    if ("/*" in chunk):
                        state.append(LEXSTATE_COMMENTBLOCK)
                        break
                        
                    if (state[-1] == LEXSTATE_COMMENTBLOCK):
                        if ("*/" in chunk):
                            state.pop()
                        continue

                    if (chunk == "BEGIN"):
                        chunk = next(lineit).rstrip()
                        if (state[-1] == LEXSTATE_MESH):
                            if (chunk == "VERTICES"):
                                state.append(LEXSTATE_VERTICES)
                            elif (chunk == "FACES"):
                                state.append(LEXSTATE_FACES)
                        elif (state[-1] == LEXSTATE_ANIMATION):
                            if (chunk == "KEYFRAME"):
                                state.append(LEXSTATE_KEYFRAME)
                                chunk = next(lineit).rstrip()

                                curkeyframe = int(chunk)
                                anims[curanim].frames[curkeyframe] = []
                        elif (state[-1] == LEXSTATE_NONE):
                            if (chunk == "MESH"):
                                state.append(LEXSTATE_MESH)
                                chunk = next(lineit).rstrip()

                                curmesh = S64Mesh(chunk)
                                meshes.append(curmesh)
                                curmesh = len(meshes)-1
                            elif (chunk == "ANIMATION"):
                                state.append(LEXSTATE_ANIMATION)
                                chunk = next(lineit).rstrip()

                                curanim = S64Anim(chunk)
                                anims.append(curanim)
                                curanim = len(anims)-1

                    elif (chunk == "END"):
                        state.pop()
                        break
                    else:
                        if (state[-1] == LEXSTATE_MESH):
                            if (chunk == "ROOT"):
                                x = float(next(lineit).rstrip())
                                y = float(next(lineit).rstrip())
                                z = float(next(lineit).rstrip())
                                meshes[curmesh].root = mathutils.Vector((x, y, z))
                            elif (chunk == "PROPERTIES"):
                                try:
                                    while True:
                                        prop = next(lineit).rstrip()
                                        meshes[curmesh].props.append(prop)
                                except StopIteration:
                                    break
                        elif (state[-1] == LEXSTATE_VERTICES):
                            x = float(chunk)
                            y = float(next(lineit).rstrip())
                            z = float(next(lineit).rstrip())
                            nx = float(next(lineit).rstrip())
                            ny = float(next(lineit).rstrip())
                            nz = float(next(lineit).rstrip())
                            cr = float(next(lineit).rstrip())
                            cg = float(next(lineit).rstrip())
                            cb = float(next(lineit).rstrip())
                            ux = float(next(lineit).rstrip())
                            uy = float(next(lineit).rstrip())

                            curvert = S64Vertex()
                            curvert.coor = mathutils.Vector((x, y, z))
                            curvert.norm = mathutils.Vector((nx, ny, nz))
                            curvert.colr = mathutils.Vector((cr, cg, cb, 1))
                            curvert.uv = mathutils.Vector((ux, -uy))
                            meshes[curmesh].verts.append(curvert)
                            curvert = len(meshes[curmesh].verts)-1
                        elif (state[-1] == LEXSTATE_FACES):
                            count = int(chunk)
                            indices = []
                            for i in range(count):
                                indices.append(int(next(lineit).rstrip()))
                            curmat = next(lineit).rstrip()
                            curface = S64Face()
                            curface.verts = indices
                            curface.mat = curmat
                            meshes[curmesh].faces.append(curface)
                            curface = len(meshes[curmesh].faces)-1
                            if not curmat in meshes[curmesh].mats:
                                meshes[curmesh].mats.append(curmat)
                            if not curmat in materials:
                                materials.append(curmat)
                        elif (state[-1] == LEXSTATE_KEYFRAME):
                            bone = chunk
                            px = float(next(lineit).rstrip())
                            py = float(next(lineit).rstrip())
                            pz = float(next(lineit).rstrip())
                            rw = float(next(lineit).rstrip())
                            rx = float(next(lineit).rstrip())
                            ry = float(next(lineit).rstrip())
                            rz = float(next(lineit).rstrip())
                            sx = float(next(lineit).rstrip())
                            sy = float(next(lineit).rstrip())
                            sz = float(next(lineit).rstrip())

                            curframedata = S64KeyFrame(bone)
                            curframedata.pos = mathutils.Vector((px, py, pz))
                            curframedata.ang = mathutils.Quaternion((rw, rx, ry, rz))
                            curframedata.scale = mathutils.Vector((sx, sy, sz))
                            anims[curanim].frames[curkeyframe].append(curframedata)
                            curframedata = len(anims[curanim].frames[curkeyframe])-1

                except StopIteration:
                    break

    return meshes, anims, materials

def GenMaterialsFromS64(materials):
    materials_blender = {}
    for n in materials:
        mat = bpy.data.materials.get(n)
        if mat is None:
            mat = bpy.data.materials.new(name=n)
            col = []
            for i in range(3):
                col.append(random.uniform(.4,1))
            col.append(1)
            mat.diffuse_color = col
        materials_blender[n] = mat
    return materials_blender

def GenMeshesFromS64(name, meshes, materials):
    meshes_blender = {}
    for m in meshes:
        mesh = bpy.data.meshes.new(m.name)  # add the new mesh
        obj = bpy.data.objects.new(mesh.name, mesh)

        # Create the collection
        if (isNewBlender()):
            if not name in bpy.data.collections:
                col = bpy.data.collections.new(name)
                bpy.context.scene.collection.children.link(col)
            else:
                col = bpy.data.collections[name]
            col.objects.link(obj)
        else:
            bpy.context.scene.objects.link(obj)

        # Initialize the bmesh
        bm = bmesh.new()
        bm.from_mesh(mesh)

        # Add vertices to the bmesh
        addedv = []
        for v in m.verts:
            addedv.append(bm.verts.new(v.coor))
        bm.verts.index_update()
        bm.verts.ensure_lookup_table()

        # Add materials to the bmesh
        addedm = {}
        for t in m.mats:
            obj.data.materials.append(materials[t])
            addedm[t] = len(obj.data.materials) - 1

        # Add faces to the bmesh, and assign materials to them
        findex = 0
        for f in m.faces:
            indices = []
            for i in f.verts:
                indices.append(addedv[i])
            bm.faces.new(indices)
            bm.faces.ensure_lookup_table()
            bm.faces[findex].material_index = addedm[f.mat]

            findex = findex + 1

        # Update the lists to make sure all's good
        bm.verts.index_update()
        bm.verts.ensure_lookup_table()
        bm.faces.index_update()
        bm.faces.ensure_lookup_table()

        # Set the UVs and vertex colors by iterating through the face loops.
        uv_layer = bm.loops.layers.uv.new("UVMap")
        color_layer = bm.loops.layers.color.new("Color")
        for face in bm.faces:
            for loop in face.loops:
                loop[uv_layer].uv = m.verts[loop.vert.index].uv
                loop[color_layer] = m.verts[loop.vert.index].colr

        # Generate the mesh object from the bmesh
        bm.to_mesh(mesh)

        # Add normals to the mesh
        normals = []
        for l in mesh.loops:
            normals.append(m.verts[l.vertex_index].norm)
        mesh.normals_split_custom_set(normals)
        mesh.use_auto_smooth = True

        meshes_blender[m.name] = obj
    return meshes_blender

def GenBonesFromS64(filename, meshes, meshes_blender):
    bones_blender = {}

    # Create the armature
    if bpy.context.active_object:
        bpy.ops.object.mode_set(mode='OBJECT',toggle=False)
    arm = bpy.data.objects.new(filename, bpy.data.armatures.new(filename))
    arm.show_in_front = True
    arm.data.display_type = 'STICK'
    if isNewBlender():
        bpy.data.collections[filename].objects.link(arm)
    else:
        bpy.context.scene.objects.link(arm)
    for i in bpy.context.selected_objects: 
        i.select_set(False)
    arm.select_set(True)
    viewscene = None
    if (isNewBlender()):
        viewscene = bpy.context.view_layer
    else:
        viewscene = bpy.context.scene
    viewscene.objects.active = arm
    bpy.ops.object.mode_set(mode='EDIT')
    bones_blender[0] = arm

    # Create the bones, give them bone properties if it exists, and parent the meshes to the armature
    for k, v in meshes_blender.items():
        if not k in bones_blender:
            bones_blender[k] = arm.data.edit_bones.new(k)
        bone = bones_blender[k]
        ind = 0
        for m in meshes:
            if m.name == k:
                break
            ind = ind + 1
        bone.matrix = mathutils.Matrix.Translation(meshes[ind].root)
        bone.tail = meshes[ind].root + mathutils.Vector((0, 5, 0))
        v.parent = arm
        for p in meshes[ind].props:
            bone[p] = 1
    bpy.ops.object.mode_set(mode='OBJECT')

    # Assign the armature modifier to each mesh, and weight the verts
    for k, v in meshes_blender.items():
        armmod = v.modifiers.new(name="Armature", type='ARMATURE')
        armmod.object = arm
        armmod.use_bone_envelopes = False
        v.vertex_groups.new(name=k)
        group = v.vertex_groups[k]
        verts = []
        for i in range(len(v.data.vertices)):
            verts.append(i)
        group.add(verts, 1, 'REPLACE')

    return bones_blender

def GenAnimsFromS64(anims, meshes_blender, bones_blender):
    bones_blender[0].animation_data_create()

    # Set the rest pose
    zeroaction = bpy.data.actions.new("None")
    bones_blender[0].animation_data.action = zeroaction
    for name, bone in bones_blender.items():
        if (bone == bones_blender[0]):
            continue
        bones_blender[0].pose.bones[name].keyframe_insert('location', frame=0)
        bones_blender[0].pose.bones[name].keyframe_insert('rotation_quaternion', frame=0)
        bones_blender[0].pose.bones[name].keyframe_insert('scale', frame=0)
    zeroaction.use_fake_user = True

    # Load the animations
    for a in anims:
        act = bpy.data.actions.new(a.name)
        bones_blender[0].animation_data.action = act
        start_frame = float("inf")
        for keyf, data in a.frames.items():
            if (keyf < start_frame):
                start_frame = keyf
            for fdata in data:
                bones_blender[0].pose.bones[fdata.bone].location = fdata.pos
                bones_blender[0].pose.bones[fdata.bone].keyframe_insert('location', frame=keyf)
                bones_blender[0].pose.bones[fdata.bone].rotation_quaternion = fdata.ang
                bones_blender[0].pose.bones[fdata.bone].keyframe_insert('rotation_quaternion', frame=keyf)
                bones_blender[0].pose.bones[fdata.bone].scale = fdata.scale
                bones_blender[0].pose.bones[fdata.bone].keyframe_insert('scale', frame=keyf)

        # Push to the NLA stack so the animation isn't lost
        tracks =  bones_blender[0].animation_data.nla_tracks
        new_track = tracks.new()
        new_track.name = a.name
        strip = new_track.strips.new(a.name, start_frame, act)
    bones_blender[0].animation_data.action = zeroaction

def CleanUp(oldactive, oldselected, oldmode):
    if (isNewBlender()):
        viewscene = bpy.context.view_layer
    else:
        viewscene = bpy.context.scene
    for obj in bpy.context.selected_objects:
        obj.select_set(False)
    for obj in oldselected:
        obj.select_set(True)
    viewscene.objects.active = oldactive
    if (not oldmode == None):
        bpy.ops.object.mode_set(mode=oldmode)

class ObjectImport(bpy.types.Operator):
    """Import a sausage-link character with animations."""
    bl_idname    = "object.import_sausage64"
    bl_label     = "Sausage64 Character Import" # The text on the import button
    bl_options   = {'REGISTER', 'UNDO'}
    filename_ext = ".S64"

    filter_glob = bpy.props.StringProperty(default="*.S64", options={'HIDDEN'}, maxlen=255)
    #setting_triangulate  = bpy.props.BoolProperty(name="Triangulate", description="Triangulate objects.", default=False)
    filepath = bpy.props.StringProperty(subtype='FILE_PATH')

    # If we are running on Blender 2.9.3 or newer, it will expect the new "annotation"
    # syntax on these parameters. In order to make newer versions of blender happy
    # we will hack these parameters into the annotation dictionary
    if isNewBlender():
        __annotations__ = {"filter_glob" : filter_glob,
                           #"setting_triangulate" : setting_triangulate,
                           "filepath" : filepath}

    def execute(self, context):
        if (isNewBlender()):
            viewscene = bpy.context.view_layer
        else:
            viewscene = bpy.context.scene
        oldselected = bpy.context.selected_objects
        oldactive = viewscene.objects.active
        oldmode = None
        if (not bpy.context.object == None):
            oldmode = bpy.context.object.mode

        try:
            filename = os.path.splitext(os.path.basename(self.filepath))[0]
            meshes, anims, materials = ParseS64(self.filepath)

            if DebugS64Export:
                for k, v in enumerate(meshes):
                    print(v)
                for k, v in enumerate(anims):
                    print(v)
                for k, v in enumerate(materials):
                    print(v)

            materials_blender = GenMaterialsFromS64(materials)
            meshes_blender = GenMeshesFromS64(filename, meshes, materials_blender)
            bones_blender = GenBonesFromS64(filename, meshes, meshes_blender)
            GenAnimsFromS64(anims, meshes_blender, bones_blender)
        except Exception:
            self.report({'ERROR'}, traceback.format_exc())
            CleanUp(oldactive, oldselected, oldmode)
            return {'CANCELLED'}

        CleanUp(oldactive, oldselected, oldmode)
        self.report({'INFO'}, 'File imported sucessfully!')
        return {'FINISHED'}

    def invoke(self, context, event):
        context.window_manager.fileselect_add(self)
        return {'RUNNING_MODAL'}

# Blender Script registration
def menu_func_import(self, context):
    self.layout.operator(ObjectImport.bl_idname, text="Sausage64 Character (.S64)")

def register():
    bpy.utils.register_class(ObjectImport)
    if (isNewBlender()):
        bpy.types.TOPBAR_MT_file_import.append(menu_func_import)
    else:
        bpy.types.INFO_MT_file_import.append(menu_func_import)

def unregister():
    bpy.utils.unregister_class(ObjectImport)
    if (isNewBlender()):
        bpy.types.TOPBAR_MT_file_import.remove(menu_func_import)
    else:
        bpy.types.INFO_MT_file_import.remove(menu_func_import)

if __name__ == "__main__":
    register()
