import sys, numpy as np
from PIL import Image
from scipy import ndimage as nd
src = np.asarray(Image.open(sys.argv[1]).convert('RGB')).astype(np.int32)
xbr = np.asarray(Image.open(sys.argv[2]).convert('RGB')).astype(np.int32)
S = 4; H, W = src.shape[0]*S, src.shape[1]*S
pal = np.unique(src.reshape(-1,3), axis=0)
# material table: original colour -> (new colour, height, texture kind, texture strength)
M = {
 (0,0,0):      ((14,16,19),   0.00, 'none', 0.0),
 (0,36,36):    ((30,52,58),   0.35, 'brush', 0.06),
 (36,72,72):   ((62,94,100),  0.65, 'speckle', 0.08),
 (72,36,0):    ((66,40,24),   0.20, 'speckle', 0.05),
 (109,72,36):  ((136,96,60),  0.55, 'scratch', 0.07),
 (109,109,109):((150,158,164),0.85, 'brush', 0.05),
 (255,255,255):((238,242,244),1.00, 'none', 0.0),
}
# labels from xBR shapes (smooth contours): nearest palette colour
d = ((xbr[:,:,None,:]-pal[None,None,:,:])**2).sum(3)
lab = d.argmin(2)
rng = np.random.default_rng(8)
def tex(kind):
    n = rng.standard_normal((H, W)).astype(np.float32)
    if kind == 'brush':  return nd.gaussian_filter(n, (0.4, 6))*3
    if kind == 'speckle':return nd.gaussian_filter(n, 0.8)*1.5
    if kind == 'scratch':return nd.gaussian_filter(n, (5, 0.5))*2.5 + nd.gaussian_filter(rng.standard_normal((H,W)).astype(np.float32),1.2)
    return np.zeros((H, W), np.float32)
col = np.zeros((H, W, 3), np.float32); height = np.zeros((H, W), np.float32)
for i, p in enumerate(pal):
    c, hgt, kind, k = M.get(tuple(int(v) for v in p), (tuple(int(v) for v in p), 0.5, 'none', 0))
    m = nd.gaussian_filter((lab == i).astype(np.float32), 0.7)
    t = 1 + tex(kind)*k
    col += m[..., None]*np.array(c, np.float32)[None, None, :]/255*t[..., None]
    height += m*hgt
h = nd.gaussian_filter(height, 1.8)
gy, gx = np.gradient(h)
shade = np.clip(-(gx*-0.6 + gy*-0.8)*14, -0.5, 0.5)
ao = np.clip((nd.gaussian_filter(height, 9)-h)*0.9, 0, 0.35)
spec = np.clip(shade-0.15, 0, None)*0.5
out = col*(1+shade[..., None]*0.9)*(1-ao[..., None]) + spec[..., None]*0.4
out = np.clip(out, 0, 1)**0.95
Image.fromarray((out*255+0.5).astype(np.uint8)).save(sys.argv[3])
