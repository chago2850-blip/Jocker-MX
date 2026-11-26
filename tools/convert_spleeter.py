#!/usr/bin/env python3
# SPDX-FileCopyrightText: 2026 Santiago Miguel y colaboradores de Jocker-MX
# SPDX-License-Identifier: GPL-3.0-or-later
#
# Convierte el modelo preentrenado Spleeter 4 stems (Deezer, licencia MIT) a ONNX para Jocker-MX.
#
#   pip install tensorflow-cpu tf_keras tf2onnx onnx onnxruntime numpy
#   curl -L https://github.com/deezer/spleeter/releases/download/v1.4.0/4stems.tar.gz | tar xz -C 4stems
#   TF_USE_LEGACY_KERAS=1 python3 tools/convert_spleeter.py 4stems jx-stems-4.onnx
#
# Entrada del ONNX: "magnitude" float32 [lotes, 512, 1024, 2]  (|STFT| 4096/1024 a 44.1 kHz, primeros 1024 bins, L/R)
# Salidas: "vocals", "drums", "bass", "other" con la misma forma (espectrogramas estimados por instrumento).
# Los pesos se guardan en float16 (con Cast a float32) para reducir el archivo a la mitad sin cambiar el cálculo.
#
# La arquitectura U-Net reproduce spleeter/model/functions/unet.py
# (Copyright (c) 2019-present, Deezer SA — MIT License).
import os
import sys

os.environ.setdefault("TF_USE_LEGACY_KERAS", "1")
os.environ.setdefault("TF_CPP_MIN_LOG_LEVEL", "2")

import numpy as np
import onnx
import onnxruntime as ort
import tensorflow as tf
import tf2onnx
import tf_keras as keras
from onnx import helper, numpy_helper, TensorProto

INSTRUMENTS = ["vocals", "drums", "bass", "other"]
T, F = 512, 1024


def apply_unet(x, name):
    L = keras.layers
    init = keras.initializers.he_uniform(seed=50)
    conv = lambda n: L.Conv2D(n, (5, 5), strides=(2, 2), padding="same", kernel_initializer=init)
    deconv = lambda n: L.Conv2DTranspose(n, (5, 5), strides=(2, 2), padding="same", kernel_initializer=init)
    act = L.ELU  # 4stems.json: conv_activation = deconv_activation = ELU
    c1 = conv(16)(x);  r1 = act()(L.BatchNormalization(axis=-1)(c1))
    c2 = conv(32)(r1); r2 = act()(L.BatchNormalization(axis=-1)(c2))
    c3 = conv(64)(r2); r3 = act()(L.BatchNormalization(axis=-1)(c3))
    c4 = conv(128)(r3); r4 = act()(L.BatchNormalization(axis=-1)(c4))
    c5 = conv(256)(r4); r5 = act()(L.BatchNormalization(axis=-1)(c5))
    c6 = conv(512)(r5); b6 = L.BatchNormalization(axis=-1)(c6); _ = act()(b6)
    u1 = act()(deconv(256)(c6)); b7 = L.BatchNormalization(axis=-1)(u1); d1 = L.Dropout(0.5)(b7)
    m1 = L.Concatenate(axis=-1)([c5, d1])
    u2 = act()(deconv(128)(m1)); b8 = L.BatchNormalization(axis=-1)(u2); d2 = L.Dropout(0.5)(b8)
    m2 = L.Concatenate(axis=-1)([c4, d2])
    u3 = act()(deconv(64)(m2)); b9 = L.BatchNormalization(axis=-1)(u3); d3 = L.Dropout(0.5)(b9)
    m3 = L.Concatenate(axis=-1)([c3, d3])
    u4 = act()(deconv(32)(m3)); b10 = L.BatchNormalization(axis=-1)(u4)
    m4 = L.Concatenate(axis=-1)([c2, b10])
    u5 = act()(deconv(16)(m4)); b11 = L.BatchNormalization(axis=-1)(u5)
    m5 = L.Concatenate(axis=-1)([c1, b11])
    u6 = act()(deconv(1)(m5)); b12 = L.BatchNormalization(axis=-1)(u6)
    u7 = L.Conv2D(2, (4, 4), dilation_rate=(2, 2), activation="sigmoid", padding="same", kernel_initializer=init)(b12)
    return tf.identity(L.Multiply()([u7, x]), name=name)


def build_and_freeze(ckpt_dir, outputs):
    tf.compat.v1.disable_eager_execution()
    keras.backend.set_learning_phase(0)
    graph = tf.Graph()
    with graph.as_default():
        x = tf.compat.v1.placeholder(tf.float32, [None, T, F, 2], name="magnitude")
        for inst in INSTRUMENTS:          # se crean las 4 redes para conservar los nombres del checkpoint
            apply_unet(x, inst)
        sess = tf.compat.v1.Session(graph=graph)
        saver = tf.compat.v1.train.Saver(var_list=[v for v in tf.compat.v1.global_variables()])
        saver.restore(sess, tf.train.latest_checkpoint(ckpt_dir) or os.path.join(ckpt_dir, "model"))
        frozen = tf.compat.v1.graph_util.convert_variables_to_constants(sess, graph.as_graph_def(), outputs)
    return frozen, sess, graph


def convert_one(ckpt_dir, inst, out_path):
    """Convierte una sola red (se ejecuta en un subproceso para limitar la memoria)."""
    frozen, _, _ = build_and_freeze(ckpt_dir, [inst])
    model, _ = tf2onnx.convert.from_graph_def(frozen, input_names=["magnitude:0"], output_names=[f"{inst}:0"], opset=17)
    for io in list(model.graph.input) + list(model.graph.output):
        io.name = io.name.replace(":0", "")
    for n in model.graph.node:
        n.input[:] = [s.replace("magnitude:0", "magnitude") for s in n.input]
        n.output[:] = [inst if s == f"{inst}:0" else s for s in n.output]
    onnx.save(model, out_path)


def weights_to_fp16(model):
    """Guarda los pesos grandes en float16 y agrega un Cast a float32 (ONNX Runtime lo pliega al cargar)."""
    g = model.graph
    new_inits, casts = [], []
    for init in list(g.initializer):
        arr = numpy_helper.to_array(init)
        if arr.dtype == np.float32 and arr.size >= 1024:
            name16 = init.name + "__fp16"
            new_inits.append(numpy_helper.from_array(arr.astype(np.float16), name16))
            casts.append(helper.make_node("Cast", [name16], [init.name], to=TensorProto.FLOAT, name=init.name + "__cast"))
            g.initializer.remove(init)
    g.initializer.extend(new_inits)
    nodes = list(g.node)
    del g.node[:]
    g.node.extend(casts + nodes)
    return model


def main():
    if len(sys.argv) == 5 and sys.argv[1] == "--one":
        convert_one(sys.argv[2], sys.argv[3], sys.argv[4])
        return
    ckpt_dir, out_path = sys.argv[1], sys.argv[2]
    import subprocess
    import tempfile
    from onnx import compose
    tmp = tempfile.mkdtemp()
    parts = []
    for inst in INSTRUMENTS:
        part = os.path.join(tmp, inst + ".onnx")
        subprocess.run([sys.executable, __file__, "--one", ckpt_dir, inst, part], check=True)
        m = onnx.load(part)
        m = compose.add_prefix(m, prefix=inst + "_", rename_inputs=False, rename_outputs=False)
        parts.append(m)
        print(f"  red «{inst}» convertida")
    # Unión manual: una sola entrada compartida "magnitude" y las 4 salidas
    g = helper.make_graph(
        nodes=[n for m in parts for n in m.graph.node],
        name="jocker_mx_stems",
        inputs=[parts[0].graph.input[0]],
        outputs=[m.graph.output[0] for m in parts],
        initializer=[i for m in parts for i in m.graph.initializer])
    merged = helper.make_model(g, opset_imports=parts[0].opset_import)
    merged.ir_version = parts[0].ir_version
    merged = weights_to_fp16(merged)
    merged.producer_name = "Jocker-MX (Spleeter 4stems, Deezer, MIT)"
    onnx.checker.check_model(merged)
    onnx.save(merged, out_path)
    print(f"ONNX guardado: {out_path} ({os.path.getsize(out_path) / 1e6:.1f} MB)")

    # Validación numérica contra TensorFlow (las 4 salidas)
    frozen, sess, graph = build_and_freeze(ckpt_dir, INSTRUMENTS)
    rng = np.random.default_rng(0)
    x = (np.abs(rng.standard_normal((1, T, F, 2))) * np.linspace(2, 0.01, F)[None, None, :, None]).astype(np.float32)
    ref = sess.run([graph.get_tensor_by_name(f"{i}:0") for i in INSTRUMENTS], {graph.get_tensor_by_name("magnitude:0"): x})
    so = ort.InferenceSession(out_path, providers=["CPUExecutionProvider"])
    got = so.run(INSTRUMENTS, {"magnitude": x})
    worst = 0.0
    for name, a, b in zip(INSTRUMENTS, ref, got):
        rel = float(np.max(np.abs(a - b)) / (np.max(np.abs(a)) + 1e-9))
        mean = float(np.mean(np.abs(a - b)) / (np.mean(np.abs(a)) + 1e-9))
        worst = max(worst, rel)
        print(f"  {name:7s} error relativo máx. {rel:.2e} · medio {mean:.2e}")
    # Los pesos en float16 introducen ~1e-3 de error típico (inaudible en las máscaras)
    if worst > 3e-2:
        sys.exit("La conversión no coincide con TensorFlow")
    print("Validación correcta")


if __name__ == "__main__":
    main()
