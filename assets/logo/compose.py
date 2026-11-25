import sys
from PIL import Image, ImageDraw, ImageFont, ImageFilter
src, outdir = sys.argv[1], sys.argv[2]
logo = Image.open(src).convert("RGBA")
MONT = "/usr/share/fonts/truetype/google-fonts/ofl/montserrat/Montserrat[wght].ttf"
def font(size, w):
    f = ImageFont.truetype(MONT, size)
    try: f.set_variation_by_axes([w])
    except Exception: pass
    return f

def gradient_text(img, xy, text, f, c1, c2):
    mask = Image.new("L", img.size, 0)
    ImageDraw.Draw(mask).text(xy, text, font=f, fill=255)
    bbox = mask.getbbox()
    grad = Image.new("RGBA", img.size)
    gd = ImageDraw.Draw(grad)
    x0, x1 = bbox[0], bbox[2]
    for x in range(x0, x1 + 1):
        t = (x - x0) / max(1, x1 - x0)
        gd.line([(x, 0), (x, img.size[1])], fill=tuple(int(c1[i] + (c2[i]-c1[i])*t) for i in range(3)) + (255,))
    img.paste(grad, (0, 0), mask)

def radial_bg(w, h):
    bg = Image.new("RGBA", (w, h), (8, 6, 16, 255))
    glow = Image.new("RGBA", (w, h), (0, 0, 0, 0))
    d = ImageDraw.Draw(glow)
    d.ellipse([w*0.05, h*0.0, w*0.95, h*1.0], fill=(70, 20, 90, 255))
    glow = glow.filter(ImageFilter.GaussianBlur(min(w, h) * 0.18))
    return Image.alpha_composite(bg, glow)

GOLD1, GOLD2 = (255, 214, 140), (214, 150, 60)
CYAN, MAG = (40, 220, 255), (255, 40, 160)

# 1) Hero horizontal (README) 1600x640
W, H = 1600, 640
hero = radial_bg(W, H)
lg = logo.resize((600, 600), Image.LANCZOS)
hero.alpha_composite(lg, (40, 20))
fh = font(128, 800); dh = ImageDraw.Draw(hero)
wjh = dh.textlength("JOCKER", font=fh)
gradient_text(hero, (684, 190), "JOCKER", fh, GOLD1, GOLD2)
gradient_text(hero, (684 + wjh, 190), "-MX", fh, CYAN, MAG)
d = ImageDraw.Draw(hero)
d.text((690, 360), "DJ  •  VIDEO  •  KARAOKE  •  STEMS  •  IA", font=font(36, 500), fill=(220, 210, 235, 255))
d.text((690, 420), "Libre · GPL-3.0+ · Windows · Linux · macOS · Android", font=font(28, 400), fill=(160, 150, 185, 255))
hero.convert("RGB").save(f"{outdir}/logo/jocker-mx-hero.png", optimize=True)

# 2) Logo vertical con nombre, fondo transparente 1024x1280
V = Image.new("RGBA", (1024, 1280), (0, 0, 0, 0))
V.alpha_composite(logo, (0, 0))
f = font(150, 800)
tmp = ImageDraw.Draw(V)
wj = tmp.textlength("JOCKER", font=f); wm = tmp.textlength("-MX", font=f)
x = (1024 - (wj + wm)) / 2
gradient_text(V, (x, 1050), "JOCKER", f, GOLD1, GOLD2)
gradient_text(V, (x + wj, 1050), "-MX", f, CYAN, MAG)
V.save(f"{outdir}/logo/jocker-mx-logo-vertical.png", optimize=True)

# 3) Logo 3D solo, transparente
logo.save(f"{outdir}/logo/jocker-mx-logo-3d.png", optimize=True)

# 4) Icono de app (cuadrado con fondo oscuro redondeado) + tamaños por plataforma
S = 1024
ic = radial_bg(S, S)
mask = Image.new("L", (S, S), 0)
ImageDraw.Draw(mask).rounded_rectangle([0, 0, S-1, S-1], radius=int(S*0.22), fill=255)
inner = logo.resize((int(S*0.92), int(S*0.92)), Image.LANCZOS)
ic.alpha_composite(inner, (int(S*0.04), int(S*0.04)))
icon = Image.new("RGBA", (S, S), (0, 0, 0, 0)); icon.paste(ic, (0, 0), mask)
icon.save(f"{outdir}/icons/jocker-mx-icon-1024.png", optimize=True)
for s in (16, 24, 32, 48, 64, 128, 256, 512):
    icon.resize((s, s), Image.LANCZOS).save(f"{outdir}/icons/jocker-mx-{s}.png", optimize=True)
icon.save(f"{outdir}/icons/jocker-mx.ico", sizes=[(16,16),(24,24),(32,32),(48,48),(64,64),(128,128),(256,256)])
icon.save(f"{outdir}/icons/jocker-mx.icns")
# Android (launcher legacy mipmaps)
for name, s in {"mdpi":48,"hdpi":72,"xhdpi":96,"xxhdpi":144,"xxxhdpi":192}.items():
    icon.resize((s, s), Image.LANCZOS).save(f"{outdir}/icons/android-mipmap-{name}.png", optimize=True)
# Android foreground adaptativo (432 con zona segura)
fg = Image.new("RGBA", (432, 432), (0,0,0,0)); fg.alpha_composite(logo.resize((288,288), Image.LANCZOS), (72,72))
fg.save(f"{outdir}/icons/android-adaptive-foreground.png", optimize=True)
print("listo")
