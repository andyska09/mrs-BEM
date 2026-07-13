import argparse
import os

import cv2
import numpy as np
import pandas as pd
import tensorflow as tf
import yaml
from tqdm import tqdm

from config.settings import Settings
from utils.loader import create_dataset
from utils.normalization import Normalization
from utils.visualization import create_plot_dict


def main():
    parser = argparse.ArgumentParser(description='Perform ablation study on all runs in a folder.')
    parser.add_argument('--load_folder', help='Folder to restore network from', required=True)
    parser.add_argument('--data_root', help='Folder/File to load data', required=True)
    parser.add_argument('--output_dir', help='Folder to save plots (png & csv)', required=True)

    args = parser.parse_args()
    load_folder_path = args.load_folder
    root_dir = args.data_root
    output_dir = args.output_dir

    assert os.path.isdir(output_dir)
    assert os.path.isdir(load_folder_path)

    settings_path = os.path.join(load_folder_path, 'settings.yaml')
    settings = Settings(settings_path, generate_log=False)

    dataset_test = create_dataset(root_dir, settings, training=False)  # , batch_size=1)

    # Loading network from file
    model_save_path = os.path.join(load_folder_path, 'best_model')
    network = tf.saved_model.load(model_save_path)
    infer = network.signatures["serving_default"]

    # get normalization coefficients that this model was trained on
    fname_yaml = os.path.join(load_folder_path, 'all.yaml')
    with open(fname_yaml, 'r') as f:
        yaml_list = yaml.load(f, Loader=yaml.SafeLoader)
        normalization = Normalization([np.array(yaml_list['means_in']), np.array(yaml_list['stds_in']),
                                       np.array(yaml_list['means_out']), np.array(yaml_list['stds_out'])])

    network_predictions_list = []
    labels_list = []
    infos_list = []

    for k, (features, labels, infos) in enumerate(tqdm(dataset_test.data)):
        # features are normalized before passing to the network
        features = normalization.normalize_inputs(features)
        predictions = infer(features)
        # features are scaled back to physical scale
        output_layer_key = list(predictions.keys())[0]
        predictions = normalization.unnormalize_labels(predictions[output_layer_key])

        infos = infos[:, -1, :]

        network_predictions_list.append(np.squeeze(predictions.numpy()))
        labels_list.append(np.squeeze(labels.numpy()))
        infos_list.append(np.squeeze(infos.numpy()))

    # plot the result
    network_predictions_np = np.vstack(network_predictions_list)
    labels_np = np.vstack(labels_list)
    infos_np = np.vstack(infos_list)

    plot_dict = create_plot_dict(network_predictions_np, labels_np, infos_np,
                                 plot_cumulative=True)

    print(network_predictions_np.shape)
    print(labels_np.shape)

    # network_predictions_np = network_predictions_np * 0.0

    # compute RMSE of predictions
    RMSE_force_x = np.sqrt(((network_predictions_np[:, 0] - labels_np[:, 0]) ** 2).mean())
    RMSE_force_y = np.sqrt(((network_predictions_np[:, 1] - labels_np[:, 1]) ** 2).mean())
    RMSE_force_z = np.sqrt(((network_predictions_np[:, 2] - labels_np[:, 2]) ** 2).mean())

    RMSE_torque = np.sqrt(((network_predictions_np[:, 3:] - labels_np[:, 3:]) ** 2).mean())
    print("RMSE force x: %f" % RMSE_force_x)
    print("RMSE force y: %f" % RMSE_force_y)
    print("RMSE force z: %f" % RMSE_force_z)
    print("RMSE torque: %f" % RMSE_torque)
    MAD_force = np.abs(network_predictions_np[:, :3] - labels_np[:, :3] - (network_predictions_np[:, :3] - labels_np[:, :3]).mean()).mean()
    MAD_torque = np.abs(network_predictions_np[:, 3:] - labels_np[:, 3:] - (network_predictions_np[:, 3:] - labels_np[:, 3:]).mean()).mean()
    print("MAD force: %f" % MAD_force)
    print("MAD torque: %f" % MAD_torque)

    # import sys
    # sys.exit()

    for idx, key in enumerate(plot_dict):
        print("Saving plot of [%s] as .png and .csv to [%s]." % (key, output_dir))
        cv2.imwrite(os.path.join(output_dir, key + ".png"), np.squeeze(plot_dict[key]))

        # export the data to csv as well
        if idx < 6:
            # create pandas dataframe and save to .csv
            df_save = pd.DataFrame()
            df_save['time'] = infos_np[:, 0]
            df_save['ground_truth_residual'] = labels_np[:, idx]
            df_save['network_prediction'] = network_predictions_np[:, idx]
            df_save.to_csv(os.path.join(output_dir, key + ".csv"), index=False)
        else:
            df_save = pd.DataFrame()
            df_save['time'] = infos_np[:, 0]
            df_save['ground_truth'] = labels_np[:, idx - 6] + infos_np[:, 2 + idx - 6]
            df_save['network_prediction'] = network_predictions_np[:, idx - 6] + infos_np[:, 2 + idx - 6]
            df_save['base_model_prediction'] = infos_np[:, 2 + idx - 6]
            df_save.to_csv(os.path.join(output_dir, key + ".csv"), index=False)


if __name__ == "__main__":
    main()
