"""Real-ESRGAN x4plus (RRDBNet) inference in numpy, loading the .pth without torch."""
import pickle, zipfile, sys
import numpy as np

class _Storage:
    def __init__(self, key, dtype): self.key, self.dtype = key, dtype

def load_pth(path):
    zf = zipfile.ZipFile(path)
    names = zf.namelist()
    prefix = names[0].split('/')[0]
    dmap = {'FloatStorage': np.float32, 'HalfStorage': np.float16}
    class U(pickle.Unpickler):
        def find_class(self, mod, name):
            if mod == 'torch._utils' and name == '_rebuild_tensor_v2':
                def rebuild(storage, offset, size, stride, *a):
                    raw = np.frombuffer(zf.read(f'{prefix}/data/{storage.key}'), dtype=storage.dtype)
                    if len(size) == 0:
                        return raw[offset]
                    item = raw.itemsize
                    return np.lib.stride_tricks.as_strided(raw[offset:], shape=size,
                                                           strides=[s * item for s in stride]).astype(np.float32)
                return rebuild
            if mod == 'torch' and name.endswith('Storage'):
                return name
            if mod == 'collections' and name == 'OrderedDict':
                import collections; return collections.OrderedDict
            return super().find_class(mod, name)
        def persistent_load(self, pid):
            # ('storage', storage_type, key, location, numel)
            st = pid[1]
            stname = st if isinstance(st, str) else getattr(st, '__name__', str(st))
            return _Storage(pid[2], dmap.get(stname, np.float32))
    return U(zf.open(f'{prefix}/data.pkl')).load()

def conv(x, w, b):
    """3x3 convolution, padding 1. x: C,H,W ; w: O,C,3,3."""
    C, H, W = x.shape
    xp = np.pad(x, ((0, 0), (1, 1), (1, 1)))
    win = np.lib.stride_tricks.sliding_window_view(xp, (3, 3), axis=(1, 2))  # C,H,W,3,3
    cols = np.ascontiguousarray(win.transpose(0, 3, 4, 1, 2)).reshape(C * 9, H * W)
    out = w.reshape(w.shape[0], -1) @ cols
    out += b[:, None]
    return out.reshape(w.shape[0], H, W)


def lrelu(x): return np.where(x > 0, x, 0.2 * x)

def up2(x): return x.repeat(2, axis=1).repeat(2, axis=2)

def run(P, x):
    g = lambda n: (P[n + '.weight'], P[n + '.bias'])
    fea = conv(x, *g('conv_first'))
    body = fea
    for i in range(23):
        rin = body
        for r in (1, 2, 3):
            pre = f'body.{i}.rdb{r}.'
            xs = [rin]
            for c in range(1, 5):
                xs.append(lrelu(conv(np.concatenate(xs, 0), *g(pre + f'conv{c}'))))
            x5 = conv(np.concatenate(xs, 0), *g(pre + 'conv5'))
            rin = x5 * 0.2 + rin
        body = rin * 0.2 + body
    fea = fea + conv(body, *g('conv_body'))
    fea = lrelu(conv(up2(fea), *g('conv_up1')))
    fea = lrelu(conv(up2(fea), *g('conv_up2')))
    out = conv(lrelu(conv(fea, *g('conv_hr'))), *g('conv_last'))
    return out

def upscale(P, img, tile=128, pad=10):
    """img: H,W,3 float 0..1 RGB -> 4H,4W,3"""
    H, W, _ = img.shape
    out = np.zeros((H * 4, W * 4, 3), np.float32)
    x = img.transpose(2, 0, 1).astype(np.float32)
    for y0 in range(0, H, tile):
        for x0 in range(0, W, tile):
            y1, x1 = min(y0 + tile, H), min(x0 + tile, W)
            ya, xa = max(0, y0 - pad), max(0, x0 - pad)
            yb, xb = min(H, y1 + pad), min(W, x1 + pad)
            r = run(P, x[:, ya:yb, xa:xb])
            out[y0 * 4:y1 * 4, x0 * 4:x1 * 4] = r[:, (y0 - ya) * 4:(y1 - ya) * 4, (x0 - xa) * 4:(x1 - xa) * 4].transpose(1, 2, 0)
            print('tile', y0, x0, flush=True)
    return np.clip(out, 0, 1)

if __name__ == '__main__':
    from PIL import Image
    sd = load_pth(sys.argv[1])
    P = sd.get('params_ema', sd.get('params', sd))
    img = np.asarray(Image.open(sys.argv[2]).convert('RGB'), np.float32) / 255.0
    out = upscale(P, img)
    Image.fromarray((out * 255 + 0.5).astype(np.uint8)).save(sys.argv[3])
