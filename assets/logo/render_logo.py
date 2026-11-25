"""Logo 3D de Jocker-MX — se renderiza con Blender (bpy) + Cycles.
Uso: python3 render_logo.py <salida.png> [resolucion] [muestras]
"""
import sys, math
import bpy
from mathutils import Vector

OUT = sys.argv[1] if len(sys.argv) > 1 else "logo.png"
RES = int(sys.argv[2]) if len(sys.argv) > 2 else 1024
SAMPLES = int(sys.argv[3]) if len(sys.argv) > 3 else 96
FONT = "/usr/share/fonts/truetype/google-fonts/ofl/archivoblack/ArchivoBlack-Regular.ttf"

bpy.ops.wm.read_factory_settings(use_empty=True)
scene = bpy.context.scene

# ---------- materiales ----------
def mat_principled(name, color, metal=0.0, rough=0.3, coat=0.0, emis=None, emis_str=0.0):
    m = bpy.data.materials.new(name)
    m.use_nodes = True
    b = m.node_tree.nodes["Principled BSDF"]
    b.inputs["Base Color"].default_value = (*color, 1)
    b.inputs["Metallic"].default_value = metal
    b.inputs["Roughness"].default_value = rough
    b.inputs["Coat Weight"].default_value = coat
    if emis:
        b.inputs["Emission Color"].default_value = (*emis, 1)
        b.inputs["Emission Strength"].default_value = emis_str
    return m, b

def gold():
    m, b = mat_principled("Oro", (1.0, 0.72, 0.32), metal=1.0, rough=0.16)
    return m

def vinyl():
    """Disco negro con surcos concéntricos (anillos procedurales -> bump + rugosidad)."""
    m, b = mat_principled("Vinilo", (0.008, 0.008, 0.012), rough=0.22, coat=0.3)
    nt = m.node_tree
    tc = nt.nodes.new("ShaderNodeTexCoord")
    wave = nt.nodes.new("ShaderNodeTexWave")
    wave.wave_type = "RINGS"
    wave.rings_direction = "SPHERICAL"
    wave.inputs["Scale"].default_value = 60
    wave.inputs["Distortion"].default_value = 0.6
    wave.inputs["Detail"].default_value = 2
    nt.links.new(tc.outputs["Object"], wave.inputs["Vector"])
    bump = nt.nodes.new("ShaderNodeBump")
    bump.inputs["Strength"].default_value = 0.18
    bump.inputs["Distance"].default_value = 0.002
    nt.links.new(wave.outputs["Fac"], bump.inputs["Height"])
    nt.links.new(bump.outputs["Normal"], b.inputs["Normal"])
    ramp = nt.nodes.new("ShaderNodeMapRange")
    ramp.inputs["To Min"].default_value = 0.14
    ramp.inputs["To Max"].default_value = 0.32
    nt.links.new(wave.outputs["Fac"], ramp.inputs["Value"])
    nt.links.new(ramp.outputs["Result"], b.inputs["Roughness"])
    return m

def label():
    """Etiqueta central: degradado profundo morado -> magenta, laca brillante."""
    m, b = mat_principled("Etiqueta", (0.2, 0.02, 0.25), rough=0.25, coat=1.0)
    nt = m.node_tree
    tc = nt.nodes.new("ShaderNodeTexCoord")
    grad = nt.nodes.new("ShaderNodeTexGradient")
    grad.gradient_type = "LINEAR"
    mp = nt.nodes.new("ShaderNodeMapping")
    mp.inputs["Rotation"].default_value = (0, 0, math.radians(-45))
    mp.inputs["Location"].default_value = (0.5, 0.0, 0)
    mp.inputs["Scale"].default_value = (1.4, 1.4, 1)
    nt.links.new(tc.outputs["Object"], mp.inputs["Vector"])
    nt.links.new(mp.outputs["Vector"], grad.inputs["Vector"])
    cr = nt.nodes.new("ShaderNodeValToRGB")
    cr.color_ramp.elements[0].color = (0.05, 0.01, 0.16, 1)   # índigo profundo
    cr.color_ramp.elements[1].color = (0.55, 0.02, 0.28, 1)   # magenta vino
    nt.links.new(grad.outputs["Fac"], cr.inputs["Fac"])
    nt.links.new(cr.outputs["Color"], b.inputs["Base Color"])
    return m

def neon():
    """Anillo de neón: degradado angular cian -> magenta -> ámbar."""
    m = bpy.data.materials.new("Neon")
    m.use_nodes = True
    nt = m.node_tree
    nt.nodes.clear()
    out = nt.nodes.new("ShaderNodeOutputMaterial")
    em = nt.nodes.new("ShaderNodeEmission")
    em.inputs["Strength"].default_value = 1.1
    tc = nt.nodes.new("ShaderNodeTexCoord")
    sep = nt.nodes.new("ShaderNodeSeparateXYZ")
    nt.links.new(tc.outputs["Object"], sep.inputs["Vector"])
    at = nt.nodes.new("ShaderNodeMath"); at.operation = "ARCTAN2"
    nt.links.new(sep.outputs["Y"], at.inputs[0]); nt.links.new(sep.outputs["X"], at.inputs[1])
    norm = nt.nodes.new("ShaderNodeMapRange")
    norm.inputs["From Min"].default_value = -math.pi
    norm.inputs["From Max"].default_value = math.pi
    nt.links.new(at.outputs[0], norm.inputs["Value"])
    cr = nt.nodes.new("ShaderNodeValToRGB")
    els = cr.color_ramp.elements
    els[0].position = 0.0; els[0].color = (0.0, 0.85, 1.0, 1)
    els[1].position = 0.5; els[1].color = (1.0, 0.05, 0.6, 1)
    e = els.new(1.0); e.color = (0.0, 0.85, 1.0, 1)
    e2 = els.new(0.78); e2.color = (1.0, 0.55, 0.05, 1)
    nt.links.new(norm.outputs["Result"], cr.inputs["Fac"])
    nt.links.new(cr.outputs["Color"], em.inputs["Color"])
    nt.links.new(em.outputs["Emission"], out.inputs["Surface"])
    return m

M_GOLD, M_VINYL, M_LABEL, M_NEON = gold(), vinyl(), label(), neon()

# ---------- geometría ----------
root = bpy.data.objects.new("Logo", None)
scene.collection.objects.link(root)

def parent(o):
    o.parent = root
    return o

def add_bevel(o, w, seg=4):
    mod = o.modifiers.new("Bevel", "BEVEL")
    mod.width = w; mod.segments = seg
    bpy.ops.object.shade_smooth()

# Disco
bpy.ops.mesh.primitive_cylinder_add(vertices=256, radius=1.0, depth=0.07, location=(0, 0, 0))
disc = parent(bpy.context.object); disc.data.materials.append(M_VINYL); add_bevel(disc, 0.012)

# Aro dorado exterior
bpy.ops.mesh.primitive_torus_add(major_radius=1.0, minor_radius=0.032, major_segments=256, minor_segments=32)
rim = parent(bpy.context.object); rim.data.materials.append(M_GOLD); bpy.ops.object.shade_smooth()

# Aro dorado interior fino (separa surcos y etiqueta)
bpy.ops.mesh.primitive_torus_add(major_radius=0.44, minor_radius=0.012, major_segments=192, minor_segments=24, location=(0, 0, 0.037))
rim2 = parent(bpy.context.object); rim2.data.materials.append(M_GOLD); bpy.ops.object.shade_smooth()

# Etiqueta central
bpy.ops.mesh.primitive_cylinder_add(vertices=192, radius=0.425, depth=0.02, location=(0, 0, 0.042))
lab = parent(bpy.context.object); lab.data.materials.append(M_LABEL); add_bevel(lab, 0.006)

# Anillo de neón flotante
bpy.ops.mesh.primitive_torus_add(major_radius=1.12, minor_radius=0.02, major_segments=256, minor_segments=16, location=(0, 0, -0.01))
ring = parent(bpy.context.object); ring.data.materials.append(M_NEON); bpy.ops.object.shade_smooth()

# Barras de forma de onda doradas en el borde (como medidor/EQ alrededor del disco)
N = 64
for i in range(N):
    a = 2 * math.pi * i / N
    h = 0.05 + 0.2 * abs(math.sin(i * 0.9) * math.cos(i * 0.37)) ** 0.8
    r = 1.22 + h / 2
    bpy.ops.mesh.primitive_cube_add(size=1, location=(r * math.cos(a), r * math.sin(a), -0.01), rotation=(0, 0, a))
    bar = parent(bpy.context.object)
    bar.scale = (h, 0.045, 0.045)
    bar.data.materials.append(M_GOLD if i % 2 == 0 else M_NEON)

# Monograma "J" dorado extruido
bpy.ops.object.text_add(location=(0, 0, 0.052))
txt = bpy.context.object
txt.data.body = "J"
fnt = bpy.data.fonts.load(FONT)
txt.data.font = fnt
txt.data.size = 0.78
txt.data.extrude = 0.035
txt.data.bevel_depth = 0.008
txt.data.bevel_resolution = 4
txt.data.align_x = "CENTER"; txt.data.align_y = "CENTER"
txt.data.materials.append(M_GOLD)
txt.location = (0.0, 0.0, 0.052 + 0.035)
parent(txt)

# Inclinación del conjunto para dar perspectiva 3D
root.rotation_euler = (math.radians(-38), math.radians(0), math.radians(18))

# ---------- cámara ----------
cam_data = bpy.data.cameras.new("Cam")
cam_data.lens = 85
cam = bpy.data.objects.new("Cam", cam_data)
scene.collection.objects.link(cam)
cam.location = (0, 0.05, 7.9)
cam.rotation_euler = (0, 0, 0)
scene.camera = cam

# ---------- luces ----------
def area(name, loc, energy, size, color=(1, 1, 1)):
    l = bpy.data.lights.new(name, "AREA"); l.energy = energy; l.size = size; l.color = color
    o = bpy.data.objects.new(name, l); scene.collection.objects.link(o)
    o.location = loc
    d = Vector((0, 0, 0)) - Vector(loc)
    o.rotation_euler = d.to_track_quat("-Z", "Y").to_euler()
    return o

area("Key", (-3.5, 3.5, 6), 1300, 3.0, (1.0, 0.95, 0.88))
area("Fill", (4, -2, 5), 250, 4.0, (0.75, 0.85, 1.0))
area("RimMagenta", (3, 4, 1.5), 500, 2.0, (1.0, 0.3, 0.7))
area("RimCyan", (-4, -3.5, 1.5), 450, 2.0, (0.3, 0.8, 1.0))
area("Top", (0, 0, 8), 300, 6.0)

# Mundo: degradado para reflejos (fondo transparente en el render)
world = bpy.data.worlds.new("W"); scene.world = world
world.use_nodes = True
wn = world.node_tree
bg = wn.nodes["Background"]
sky = wn.nodes.new("ShaderNodeTexGradient"); sky.gradient_type = "SPHERICAL"
tc = wn.nodes.new("ShaderNodeTexCoord")
cr = wn.nodes.new("ShaderNodeValToRGB")
cr.color_ramp.elements[0].color = (0.10, 0.06, 0.04, 1)
cr.color_ramp.elements[1].color = (1.0, 0.92, 0.80, 1)
wn.links.new(tc.outputs["Generated"], sky.inputs["Vector"])
wn.links.new(sky.outputs["Fac"], cr.inputs["Fac"])
wn.links.new(cr.outputs["Color"], bg.inputs["Color"])
bg.inputs["Strength"].default_value = 1.4

# ---------- render ----------
scene.render.engine = "CYCLES"
scene.cycles.device = "CPU"
scene.cycles.samples = SAMPLES
scene.cycles.use_denoising = True
scene.render.film_transparent = True
scene.render.resolution_x = RES
scene.render.resolution_y = RES
scene.render.image_settings.file_format = "PNG"
scene.render.image_settings.color_mode = "RGBA"
scene.view_settings.view_transform = "AgX"
scene.view_settings.look = "AgX - Punchy"
scene.render.filepath = OUT
bpy.ops.render.render(write_still=True)
print("OK", OUT)
