"""Where does the per-batch time go? Producer vs normalization vs train_step.

    python profile_train_step.py data/bem-ctrl/train/bem-ctrl_2021-02-23-19-04-50_seg_2.csv
"""
import sys
import time

import tensorflow as tf

from config.settings import Settings
from learner import Learner
from utils.loader import create_dataset
from utils.normalization import Normalization

N = 200
WARMUP = 5


def bench(label, it, fn):
    for _ in range(WARMUP):
        fn(next(it))
    t = time.perf_counter()
    for _ in range(N):
        fn(next(it))
    dt = (time.perf_counter() - t) / N * 1e3
    print("%-42s %7.2f ms/batch" % (label, dt))
    return dt


def main():
    # optional 2nd arg: "cpu" hides the Metal GPU, "growth" replaces the 1000 MB cap
    mode = sys.argv[2] if len(sys.argv) > 2 else ""
    gpus = tf.config.list_physical_devices('GPU')
    if mode == "cpu":
        tf.config.set_visible_devices([], 'GPU')
    elif mode == "growth" and gpus:
        tf.config.experimental.set_memory_growth(gpus[0], True)
    print("mode=%s gpus=%s" % (mode or "default", gpus))

    settings = Settings("config/bem_settings.yaml", generate_log=False)
    if mode == "gpu":
        settings.use_gpu = True
    learner = Learner(settings)

    if mode == "batch":
        for bs in [32, 128, 512, 2048]:
            ds = create_dataset(sys.argv[1], settings, training=True, batch_size=bs)
            it = iter(ds.data)
            for _ in range(WARMUP):
                next(it)
            n = max(20, N * 128 // bs)
            t = time.perf_counter()
            for _ in range(n):
                next(it)
            per_batch = (time.perf_counter() - t) / n * 1e3
            print("batch %5d: %7.2f ms/batch   %6.3f ms/1k samples"
                  % (bs, per_batch, per_batch / bs * 1000))
        return

    dataset = create_dataset(sys.argv[1], settings, training=True)
    learner.normalization = Normalization(dataset.get_normalization())
    norm = learner.normalization

    @tf.function
    def step_nograd(inputs, labels):
        with tf.GradientTape() as tape:
            predictions = learner.network(inputs)
            forces_loss = learner.forces_loss(labels, predictions)
            torques_loss = learner.torques_loss(labels, predictions)
            loss = (settings.weight_force * forces_loss
                    + settings.weight_torque * torques_loss)
        gradients = tape.gradient(loss, learner.network.trainable_variables)
        learner.optimizer.apply_gradients(zip(gradients, learner.network.trainable_variables))
        return loss

    def normalize(b):
        return norm.normalize_inputs(b[0]), norm.normalize_labels(b[1])

    a = bench("(a) producer only", iter(dataset.data), lambda b: None)
    b = bench("(b) + normalization (python, eager)", iter(dataset.data), normalize)
    c = bench("(c) + train_step (returns gradients)", iter(dataset.data),
              lambda x: learner.train_step(*normalize(x)))
    d = bench("(d) + train_step (no gradient return)", iter(dataset.data),
              lambda x: step_nograd(*normalize(x)))

    print("\nproducer %.0f%% | normalization %.0f%% | train_step %.0f%%"
          % (a / c * 100, (b - a) / c * 100, (c - b) / c * 100))
    print("cost of returning gradients to python: %.2f ms/batch" % (c - d))


if __name__ == "__main__":
    main()
