import os

import matplotlib.pyplot as plt
import numpy as np
import pandas as pd
import seaborn as sns
import tensorflow as tf
from tqdm import tqdm

try:
    from window_generator import WindowGenerator
    from visualization import plot_window
except:
    from .window_generator import WindowGenerator
    from .visualization import plot_window


def create_dataset(root_path, settings, training=True, batch_size=None):
    dataset = QuadDataset(root_path, settings, training, batch_size)
    return dataset


class Dataset:
    """
    Base Dataset Class
    """

    def __init__(self, root_path, config, training=True, batch_size=None, debug=False):
        print("------------------------------------------")
        print('Building %s Dataset' % ("Training" if training else "Validation"))
        print("------------------------------------------")
        self.config = config
        self.root_path = root_path
        self.training = training
        self.batch_size = batch_size
        self.normalization_coeffs = None
        self.debug = debug
        self.experiments = []
        self.input_features = []
        self.label_features = []
        file_rootname = config.base_type + '_'
        if os.path.isdir(root_path):
            for root, dirs, files in os.walk(root_path, topdown=True, followlinks=True):
                for name in files:
                    if name.startswith(file_rootname):
                        self.experiments.append(os.path.join(root, name))
        elif os.path.isfile(root_path):
            if file_rootname in root_path:
                self.experiments.append(root_path)
        else:
            assert False, "Provided dataset root is neither a file nor a directory!"

        self.num_experiments = len(self.experiments)
        assert self.num_experiments > 0, 'No valid data found!'
        print('Dataset contains %d experiments.' % self.num_experiments)
        self.data_format = 'csv'

        self.dataset = None
        # numpy arrays to store the raw data and compute mean/std for normalization
        self.raw_inputs = None
        self.raw_labels = None
        print("Decoding data...")
        for exp in tqdm(self.experiments):
            if self.dataset is None:
                self.dataset = self._decode_experiment(exp)
            else:
                self.dataset = self.dataset.concatenate(self._decode_experiment(exp))

        if len(self.dataset) == 0:
            raise IOError("Did not find any file in the dataset folder")

        # this computes the normalization
        self._preprocess_dataset()

        # compute correlation of labels w.r.t. input features
        #  for j in range(3):
        #  for i in range(6):
        #  correlation = np.corrcoef(self.raw_inputs[:, j], self.raw_labels[:, i])[0, 1]
        #  if abs(correlation) > 0.7:
        #  print("label = [%s], feature = [%s], corr = %.3f" % (
        #  self.label_features[i], self.input_features[j], correlation))

        print('Found {} samples assembled from {} raw data points belonging to {} experiments:'.format(
            len(self.dataset), self.raw_inputs.shape[0], self.num_experiments))

    def get_normalization(self):
        return self._get_normalization()

    def _get_normalization(self):
        raise NotImplementedError

    def _preprocess_dataset(self):
        raise NotImplementedError

    def _decode_experiment(self, dir_subpath):
        raise NotImplementedError

    @property
    def data(self):
        return self.dataset


class QuadDataset(Dataset):
    def __init__(self, root_path, config, training=True, batch_size=None):
        super(QuadDataset, self).__init__(root_path, config, training, batch_size)

    def _decode_experiment(self, data_file):
        '''
        Generates a TF dataset from a .csv file.
        The TF dataset contains samples of the format (features, labels).
        The features and labels have the following shapes:
        Features: [batch_size, window_size_features, feature_dim]
        Labels: [batch_size, window_size_labels, label_dim]

        :param data_file: absolute path of the csv file
        :return: TF dataset
        '''
        header = ["t", "ang acc x", "ang acc y", "ang acc z", \
                  "ang vel x", "ang vel y", "ang vel z", \
                  "quat x", "quat y", "quat z", "quat w", \
                  "acc x", "acc y", "acc z", \
                  "vel x", "vel y", "vel z", \
                  "pos x", "pos y", "pos z", \
                  "mot 1", "mot 2", "mot 3", "mot 4", \
                  "dmot 1", "dmot 2", "dmot 3", "dmot 4", "vbat", \
                  "model_fx", "model_fy", "model_fz", \
                  "model_tx", "model_ty", "model_tz", \
                  "residual_fx", "residual_fy", "residual_fz", \
                  "residual_tx", "residual_ty", "residual_tz"]
        if self.debug:
            print("Decoding [%s]" % data_file)
        assert os.path.isfile(data_file), "Not Found data file"
        df = pd.read_csv(data_file, delimiter=',', names=header)

        if self.debug:
            # let's have a look at the data...
            pd.set_option("max_columns", None)  # show all cols
            pd.set_option('max_colwidth', None)  # show full width of showing cols
            print(df.describe().transpose())
            print("Average sampling frequency of data is %.6f" % (1.0 / np.mean(np.diff(df["t"].values))))
            # plot the prepared data
            self._visualize_dataset(df)

        # if loader is instantiated with a batch size, we overwrite the batch size of settings.yaml
        if self.batch_size is not None:
            batch_size = self.batch_size
        else:
            batch_size = self.config.batch_size

        self.input_features = []
        if self.config.input_use_pos:
            self.input_features += ["pos x",
                                    "pos y",
                                    "pos z"]
        if self.config.input_use_att:
            self.input_features += ["quat w",
                                    "quat x",
                                    "quat y",
                                    "quat z"]
        if self.config.input_use_angvel:
            self.input_features += ["ang vel x",
                                    "ang vel y",
                                    "ang vel z"]
        if self.config.input_use_linvel:
            self.input_features += ["vel x",
                                    "vel y",
                                    "vel z"]
        if self.config.input_use_motors:
            self.input_features += ["mot 1",
                                    "mot 2",
                                    "mot 3",
                                    "mot 4"]
        if self.config.input_use_base_force:
            self.input_features += ["model_fx",
                                    "model_fy",
                                    "model_fz"]
        if self.config.input_use_base_torque:
            self.input_features += ["model_tx",
                                    "model_ty",
                                    "model_tz"]

        self.label_features = ["residual_fx",
                               "residual_fy",
                               "residual_fz",
                               "residual_tx",
                               "residual_ty",
                               "residual_tz"]

        # not used for training, but adding them to the loader for potential plotting / inspection
        self.info_features = ["t",
                              "vbat",
                              "model_fx",
                              "model_fy",
                              "model_fz",
                              "model_tx",
                              "model_ty",
                              "model_tz",
                              "residual_fx",  # 8
                              "residual_fy",
                              "residual_fz",
                              "residual_tx",
                              "residual_ty",
                              "residual_tz"
                              ]

        valid_mask = None
        if getattr(self.config, 'max_speed', 0):
            speed = np.linalg.norm(df[["vel x", "vel y", "vel z"]].values, axis=1)
            valid_mask = speed <= self.config.max_speed

        my_window = WindowGenerator(input_width=self.config.history_len,
                                    label_width=self.config.label_len,
                                    shift=self.config.label_shift,
                                    dataframe=df,
                                    feature_columns=self.input_features,
                                    label_columns=self.label_features,
                                    info_columns=self.info_features,
                                    sampling_rate=self.config.sampling_rate,
                                    batch_size=batch_size,
                                    shuffle=self.training,
                                    valid_mask=valid_mask)
        if self.debug:
            print(my_window)
            # we'll define the windows we want to use for training
            # Stack three slices, the length of the total window:
            example_window = tf.stack([np.array(df[:my_window.total_window_size]),
                                       np.array(df[100:100 + my_window.total_window_size]),
                                       np.array(df[200:200 + my_window.total_window_size])])

            print(example_window.shape)
            example_inputs, example_labels, example_infos = my_window.split_window(example_window)

            print('All shapes are: (batch, time, features)')
            print(f'Raw dataframe shape: {df.values.shape}')
            print(f'Window shape: {example_window.shape}')
            print(f'Inputs shape: {example_inputs.shape}')
            print(f'labels shape: {example_labels.shape}')
            print(f'infos shape: {example_infos.shape}')

            my_window.example = example_inputs, example_labels, example_infos

            my_window.plot(plot_col='vel x')
            plt.show()

        dataset = my_window.get_dataset
        if self.debug:
            for example_inputs, example_labels, example_infos in dataset.take(1):
                print(f'Inputs shape (batch, history_len, feature_dim): {example_inputs.shape}')
                print(f'Labels shape (batch, history_len, feature_dim): {example_labels.shape}')
                print(f'Infos shape (batch, history_len, feature_dim): {example_infos.shape}')
                # plot the generated data
                plot_window(example_inputs, example_labels, example_infos)
                pass

        input_features_v = df[self.input_features].values
        label_features_v = df[self.label_features].values
        if valid_mask is not None:
            # normalization constants must not see rows above the cut
            input_features_v = input_features_v[valid_mask]
            label_features_v = label_features_v[valid_mask]

        if self.raw_inputs is None:
            self.raw_inputs = input_features_v
            self.raw_labels = label_features_v
        else:
            self.raw_inputs = np.concatenate([self.raw_inputs, input_features_v], axis=0)
            self.raw_labels = np.concatenate([self.raw_labels, label_features_v], axis=0)

        return dataset

    def _preprocess_dataset(self):
        if self.normalization_coeffs is None:
            self.input_mean = np.mean(self.raw_inputs, axis=0)
            self.input_std = np.std(self.raw_inputs, axis=0)
            self.output_mean = np.mean(self.raw_labels, axis=0)
            self.output_std = np.std(self.raw_labels, axis=0)
        else:
            self.input_mean = self.normalization_coeffs[0]
            self.input_std = self.normalization_coeffs[1]
            self.output_mean = self.normalization_coeffs[2]
            self.output_std = self.normalization_coeffs[3]

        # print some infos about the dataset
        ang_speed = np.sqrt(np.square(self.raw_inputs[:, 0])
                            + np.square(self.raw_inputs[:, 1])
                            + np.square(self.raw_inputs[:, 2]))
        print("Highest angular speed observed in the dataset: %.3f rad/s" % np.max(ang_speed))
        lin_speed = np.sqrt(np.square(self.raw_inputs[:, 3])
                            + np.square(self.raw_inputs[:, 4])
                            + np.square(self.raw_inputs[:, 5]))
        print("Highest linear speed observed in the dataset: %.3f m/s" % np.max(lin_speed))
        force_norm = np.sqrt(np.square(self.raw_labels[:, 0])
                             + np.square(self.raw_labels[:, 1])
                             + np.square(self.raw_labels[:, 2]))
        print("Highest residual force observed in the dataset: %.3f N" % np.max(force_norm))
        torque_norm = np.sqrt(np.square(self.raw_labels[:, 3])
                              + np.square(self.raw_labels[:, 4])
                              + np.square(self.raw_labels[:, 5]))
        print("Highest residual torque observed in the dataset: %.3f Nm" % np.max(torque_norm))

    def _get_normalization(self):
        return self.input_mean, self.input_std, self.output_mean, self.output_std

    @staticmethod
    def _visualize_dataset(dataframe):
        df_std = dataframe.melt(var_name='Column', value_name='Normalized')
        plt.figure(figsize=(12, 6))
        ax = sns.violinplot(x='Column', y='Normalized', data=df_std)
        _ = ax.set_xticklabels(dataframe.keys(), rotation=90)
        plt.show()


if __name__ == "__main__":
    class DebugSettings:
        def __init__(self):
            self.base_type = 'bem'
            self.batch_size = 32
            self.quad_mass = 0.772
            self.quad_inertia = [0.0025, 0.0021, 0.0043]
            self.label_len = 1
            self.label_shift = 0
            self.history_len = 500
            self.sampling_rate = 1
            self.input_use_angvel = True
            self.input_use_linvel = True
            self.input_use_motors = True
            self.input_use_base_force = True
            self.input_use_base_torque = True
            self.feature_dim = 12


    settings = DebugSettings()
    create_dataset("/home/elia/leonard_dataset/bem/bem_2021-02-03-13-43-38_seg_1.csv", settings=settings, training=True)
