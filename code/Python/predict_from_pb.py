import argparse
import os

import numpy as np
import tensorflow as tf
import yaml

from config.settings import Settings
from utils.normalization import Normalization


def main():
    parser = argparse.ArgumentParser(description='Test predicting from a .pb file.')
    parser.add_argument('--log_folder', help='Folder to restore network from', required=True)

    args = parser.parse_args()
    log_folder_path = args.log_folder

    settings_path = os.path.join(log_folder_path, 'settings.yaml')
    settings = Settings(settings_path, generate_log=False)

    # Loading network from file
    model_save_path = os.path.join(log_folder_path, 'best_model')
    network = tf.saved_model.load(model_save_path)
    infer = network.signatures["serving_default"]

    # get normalization coefficients that this model was trained on
    fname_yaml = os.path.join(log_folder_path, 'all.yaml')
    with open(fname_yaml, 'r') as f:
        yaml_list = yaml.load(f, Loader=yaml.SafeLoader)
        normalization = Normalization([np.array(yaml_list['means_in']), np.array(yaml_list['stds_in']),
                                       np.array(yaml_list['means_out']), np.array(yaml_list['stds_out'])])

    # Prepare input tensor
    input_tensor_np = np.ones((1, settings.history_len, settings.feature_dim))

    input_tensor = tf.constant(input_tensor_np, dtype=tf.float32)

    input_features = normalization.normalize_inputs(input_tensor)

    prediction = infer(input_features)
    prediction_notnormalized = infer(input_tensor)

    # the prediction is a python dictionary, get first element of dictionary
    values_view = prediction.values()
    value_iterator = iter(values_view)
    first_value = next(value_iterator)
    output = normalization.unnormalize_labels(first_value)

    values_view_notnormalized = prediction_notnormalized.values()
    value_iterator_notnormalized = iter(values_view_notnormalized)
    first_value_notnormalized = next(value_iterator_notnormalized)
    # this should give the same result as if one runs `test.py` with the respective settings!
    # (make sure to load the right ckpt though...)
    print("This is the unnormalized output based on a normalized ones input\n")
    print(output)
    print("This is the output based on a ones input\n")
    print(first_value_notnormalized)


if __name__ == "__main__":
    main()
