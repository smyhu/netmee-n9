# NetMee icon: serif N rising over a dark horizon with a stylized 9 in the
# lower right corner, on our dark N9 background with the arc gloss (as in
# MeeTranslate and Lyrion). Usage: python3 icon.py background80.png outdir
from PIL import Image, ImageDraw, ImageFont, ImageFilter
import sys, os
K = 8
W = 80 * K
LIGHT = (231, 255, 205)   # leading yellow-green (MeeTranslate, Lyrion)
MID = (161, 190, 145)
GLOW = (120, 170, 100)
STROKE = (42, 42, 44)    # middle band of the background
FONTS = '/usr/share/fonts/truetype/'
base = Image.open(sys.argv[1]).convert('RGBA')
out_dir = sys.argv[2]

def text_mask(txt, font, size, pos):
    m = Image.new('L', (W, W), 0)
    ImageDraw.Draw(m).text(pos, txt, font=ImageFont.truetype(font, size), fill=255, anchor='ms')
    return m

def make(name, n_color, nine_color, planet, rim):
    img = base.resize((W, W), Image.BICUBIC)
    alpha = img.split()[3]
    n = text_mask('N', FONTS + 'liberation/LiberationSerif-Bold.ttf', int(W * 0.72), (W * 0.47, W * 0.70))
    glow = n.filter(ImageFilter.GaussianBlur(K * 1.6)).point(lambda v: int(v * 0.55))
    img = Image.composite(Image.new('RGBA', (W, W), GLOW + (255,)), img, glow)
    img = Image.composite(Image.new('RGBA', (W, W), n_color + (255,)), img, n)
    hz = Image.new('L', (W, W), 0)
    ImageDraw.Draw(hz).ellipse([-W * 0.6, W * 0.66, W * 1.6, W * 2.4], fill=255)
    rimm = Image.new('L', (W, W), 0)
    ImageDraw.Draw(rimm).ellipse([-W * 0.6, W * 0.66 - K * 2.2, W * 1.6, W * 2.4], fill=255)
    img = Image.composite(Image.new('RGBA', (W, W), rim + (255,)), img, rimm)
    img = Image.composite(Image.new('RGBA', (W, W), planet + (255,)), img, hz)
    nine = text_mask('9', FONTS + 'dejavu/DejaVuSans-Bold.ttf', int(W * 0.46), (W * 0.76, W * 0.92))
    edge = nine.filter(ImageFilter.MaxFilter(2 * K + 1))
    img = Image.composite(Image.new('RGBA', (W, W), STROKE + (255,)), img, edge)
    img = Image.composite(Image.new('RGBA', (W, W), nine_color + (255,)), img, nine)
    img.putalpha(alpha)
    small = img.resize((80, 80), Image.LANCZOS)
    small.save(os.path.join(out_dir, name + '.png'))
    return small

final = make('netmee-n9', LIGHT, LIGHT, (14, 14, 16), MID)
sheet = Image.new('RGBA', (300, 230), (0, 0, 0, 255))
sheet.alpha_composite(final.resize((180, 180), Image.LANCZOS), (20, 20))
sheet.alpha_composite(final, (210, 120))
sheet.save(os.path.join(out_dir, 'icon-preview.png'))
