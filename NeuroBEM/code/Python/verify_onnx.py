import argparse
import os

import numpy as np
import onnxruntime as ort
import tensorflow as tf


def main():
    p = argparse.ArgumentParser()
    p.add_argument("--log_folder", required=True)
    args = p.parse_args()

    x = np.ones((1, 20, 10), dtype=np.float32)

    tf_model = tf.saved_model.load(os.path.join(args.log_folder, "best_model"))
    infer = tf_model.signatures["serving_default"]
    tf_out = np.array(list(infer(tf.constant(x)).values())[0]).squeeze()

    sess = ort.InferenceSession(os.path.join(args.log_folder, "best_model", "network.onnx"))
    onnx_out = sess.run(None, {sess.get_inputs()[0].name: x})[0].squeeze()

    print("TF   :", tf_out)
    print("ONNX :", onnx_out)
    print("max abs diff: %.2e" % np.abs(tf_out - onnx_out).max())


if __name__ == "__main__":
    main()
