#!/usr/bin/env python3
# SPDX-FileCopyrightText: 2026 Santiago Miguel y colaboradores de Jocker-MX
# SPDX-License-Identifier: GPL-3.0-or-later
#
# Exporta los pesos del modelo preentrenado Spleeter 4 stems (Deezer, licencia MIT) al formato propio
# de Jocker-MX (jx-stems-4.bin), que ejecuta el motor de inferencia en C++ de src/stems/ sin
# dependencias externas. También genera valores de referencia para las pruebas automáticas.
#
#   pip install tensorflow-cpu tf_keras numpy
#   mkdir 4stems && curl -L https://github.com/deezer/spleeter/releases/download/v1.4.0/4stems.tar.gz | tar xz -C 4stems
#   python3 tools/export_stems_model.py 4stems jx-stems-4.bin tests/data/stems-referencia.json
#
# Formato jx-stems-4.bin (little-endian):
#   "JXSTEMS1" · uint32 n_instrumentos · por instrumento: uint32 len + nombre UTF-8 · uint32 n_tensores ·
#   por tensor: uint8 tipo (0 = float16, 1 = float32) · uint32 ndim · uint32 dims[ndim] · datos
# Orden de tensores por instrumento (TF/Keras, NHWC):
#   6 × [conv kernel(5,5,cin,cout) f16, conv bias f32, bn escala f32, bn desplazamiento f32]
#   6 × [deconv kernel(5,5,cout,cin) f16, deconv bias f32, bn escala f32, bn desplazamiento f32]
#   final kernel(4,4,1,2) f32, final bias f32
# La U-Net reproduce spleeter/model/functions/unet.py (Copyright (c) 2019-present, Deezer SA — MIT).
import json
import os
import struct
import sys

os.environ.setdefault("TF_USE_LEGACY_KERAS", "1")
os.environ.setdefault("TF_CPP_MIN_LOG_LEVEL", "2")

import numpy as np
import tensorflow as tf
import tf_keras as keras

INSTRUMENTS = ["vocals", "drums", "bass", "other"]
T, F = 512, 1024
EPS = 1e-3  # épsilon de BatchNormalization en Keras


def apply_unet(x):
    L = keras.layers
    act = L.ELU
    layers = {"conv": [], "bn_enc": [], "deconv": [], "bn_dec": [], "final": None}
    def conv(n):
        l = L.Conv2D(n, (5, 5), strides=(2, 2), padding="same"); layers["conv"].append(l); return l
    def deconv(n):
        l = L.Conv2DTranspose(n, (5, 5), strides=(2, 2), padding="same"); layers["deconv"].append(l); return l
    def bn(kind):
        l = L.BatchNormalization(axis=-1); layers[kind].append(l); return l
    c1 = conv(16)(x);  r1 = act()(bn("bn_enc")(c1))
    c2 = conv(32)(r1); r2 = act()(bn("bn_enc")(c2))
    c3 = conv(64)(r2); r3 = act()(bn("bn_enc")(c3))
    c4 = conv(128)(r3); r4 = act()(bn("bn_enc")(c4))
    c5 = conv(256)(r4); r5 = act()(bn("bn_enc")(c5))
    c6 = conv(512)(r5); b6 = bn("bn_enc")(c6); _ = act()(b6)
    u1 = act()(deconv(256)(c6)); b7 = bn("bn_dec")(u1); d1 = L.Dropout(0.5)(b7)
    m1 = L.Concatenate(axis=-1)([c5, d1])
    u2 = act()(deconv(128)(m1)); b8 = bn("bn_dec")(u2); d2 = L.Dropout(0.5)(b8)
    m2 = L.Concatenate(axis=-1)([c4, d2])
    u3 = act()(deconv(64)(m2)); b9 = bn("bn_dec")(u3); d3 = L.Dropout(0.5)(b9)
    m3 = L.Concatenate(axis=-1)([c3, d3])
    u4 = act()(deconv(32)(m3)); b10 = bn("bn_dec")(u4)
    m4 = L.Concatenate(axis=-1)([c2, b10])
    u5 = act()(deconv(16)(m4)); b11 = bn("bn_dec")(u5)
    m5 = L.Concatenate(axis=-1)([c1, b11])
    u6 = act()(deconv(1)(m5)); b12 = bn("bn_dec")(u6)
    fin = L.Conv2D(2, (4, 4), dilation_rate=(2, 2), activation="sigmoid", padding="same")
    layers["final"] = fin
    u7 = fin(b12)
    return L.Multiply()([u7, x]), layers


def reference_input():
    t = np.arange(T)[:, None, None]
    f = np.arange(F)[None, :, None]
    c = np.arange(2)[None, None, :]
    x = np.abs(np.sin(0.37 * t + 0.011 * f * (c + 1))) * (1.5 - f / F) + 0.05
    return x[None].astype(np.float32)


def main():
    ckpt, out_bin, out_ref = sys.argv[1], sys.argv[2], sys.argv[3]
    tf.compat.v1.disable_eager_execution()
    keras.backend.set_learning_phase(0)
    x = tf.compat.v1.placeholder(tf.float32, [None, T, F, 2], name="magnitude")
    outs, nets = [], []
    for _ in INSTRUMENTS:
        o, layers = apply_unet(x)
        outs.append(o); nets.append(layers)
    sess = tf.compat.v1.Session()
    tf.compat.v1.train.Saver().restore(sess, tf.train.latest_checkpoint(ckpt) or os.path.join(ckpt, "model"))

    def bn_affine(l):
        g, b, m, v = sess.run(l.weights)
        s = g / np.sqrt(v + EPS)
        return s.astype(np.float32), (b - m * s).astype(np.float32)

    with open(out_bin, "wb") as f:
        def tensor(a, fp16):
            a = np.ascontiguousarray(a, dtype=np.float16 if fp16 else np.float32)
            f.write(struct.pack("<BI", 0 if fp16 else 1, a.ndim))
            f.write(struct.pack(f"<{a.ndim}I", *a.shape))
            f.write(a.tobytes())
        f.write(b"JXSTEMS1")
        f.write(struct.pack("<I", len(INSTRUMENTS)))
        for name, layers in zip(INSTRUMENTS, nets):
            nb = name.encode()
            f.write(struct.pack("<I", len(nb))); f.write(nb)
            f.write(struct.pack("<I", 6 * 4 + 6 * 4 + 2))
            for conv, bn in zip(layers["conv"], layers["bn_enc"]):
                k, b = sess.run(conv.weights)
                tensor(k, True); tensor(b, False)
                s, sh = bn_affine(bn); tensor(s, False); tensor(sh, False)
            for dc, bn in zip(layers["deconv"], layers["bn_dec"]):
                k, b = sess.run(dc.weights)
                tensor(k, True); tensor(b, False)
                s, sh = bn_affine(bn); tensor(s, False); tensor(sh, False)
            k, b = sess.run(layers["final"].weights)
            tensor(k, False); tensor(b, False)
    print(f"Pesos guardados: {out_bin} ({os.path.getsize(out_bin) / 1e6:.1f} MB)")

    xi = reference_input()
    res = sess.run(outs, {x: xi})
    idx = [(0, 0, 0), (17, 33, 1), (100, 200, 0), (255, 511, 1), (300, 700, 0), (511, 1023, 1), (128, 64, 0), (400, 900, 1)]
    ref = {"input": "abs(sin(0.37*t + 0.011*f*(c+1))) * (1.5 - f/1024) + 0.05", "instruments": {}}
    for name, r in zip(INSTRUMENTS, res):
        r = r[0]
        ref["instruments"][name] = {
            "mean": float(r.mean()), "mean_abs": float(np.abs(r).mean()),
            "samples": [[t, fq, c, float(r[t, fq, c])] for t, fq, c in idx]}
    with open(out_ref, "w") as f:
        json.dump(ref, f, indent=1)
    print(f"Referencia guardada: {out_ref}")


if __name__ == "__main__":
    main()
