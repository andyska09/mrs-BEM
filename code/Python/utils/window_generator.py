import matplotlib.pyplot as plt
import numpy as np
import tensorflow as tf


class WindowGenerator:
    def __init__(self, input_width, label_width, shift, dataframe=None, feature_columns=None, label_columns=None,
                 info_columns=None, sampling_rate=1, batch_size=32, shuffle=False):
        assert shift == 0, "shift != might not be supported"
        # Store the raw data.
        self.dataframe = dataframe

        self.debug = False

        # Work out the feature column indices.
        self.feature_columns = feature_columns
        if feature_columns is not None:
            self.feature_columns_indices = {name: i for i, name in
                                            enumerate(feature_columns)}
        # Work out the label column indices.
        self.label_columns = label_columns
        if label_columns is not None:
            self.label_columns_indices = {name: i for i, name in
                                          enumerate(label_columns)}
        # Work out the info column indices.
        self.info_columns = info_columns
        if info_columns is not None:
            self.info_columns_indices = {name: i for i, name in
                                         enumerate(info_columns)}

        self.column_indices = {name: i for i, name in
                               enumerate(dataframe.columns)}

        # Work out the window parameters.
        self.input_width = input_width
        self.label_width = label_width
        self.shift = shift
        self.sampling_rate = sampling_rate

        self.total_window_size = input_width + shift

        self.info_width = self.total_window_size

        self.input_slice = slice(0, input_width)
        self.input_indices = np.arange(self.total_window_size)[self.input_slice]

        self.label_start = self.total_window_size - self.label_width
        self.labels_slice = slice(self.label_start, None)
        self.label_indices = np.arange(self.total_window_size)[self.labels_slice]

        self.info_slice = slice(0, self.total_window_size)
        self.info_indices = np.arange(self.total_window_size)[self.info_slice]

        self.batch_size = batch_size
        self.shuffle = shuffle

    def __repr__(self):
        return '\n'.join([
            f'===========================================',
            f'Total window size: {self.total_window_size}',
            f'Input indices: {self.input_indices}',
            f'Label indices: {self.label_indices}',
            f'Info indices: {self.info_indices}',
            f'Feature column name(s): {self.feature_columns}',
            f'Label column name(s): {self.label_columns}',
            f'Info column name(s): {self.info_columns}',
            f'==========================================='])

    def split_window(self, features):
        '''
        Splits dataframe into features, labels and infos.
        :param features:
        :return:
        '''
        if self.debug:
            print("Splitting window!")
            print(features.shape)
        inputs = features[:, self.input_slice, :]
        labels = features[:, self.labels_slice, :]
        infos = features[:, self.info_slice, :]
        if self.feature_columns is not None:
            inputs = tf.stack(
                [inputs[:, :, self.column_indices[name]] for name in self.feature_columns],
                axis=-1)
        if self.label_columns is not None:
            labels = tf.stack(
                [labels[:, :, self.column_indices[name]] for name in self.label_columns],
                axis=-1)
        if self.info_columns is not None:
            infos = tf.stack(
                [infos[:, :, self.column_indices[name]] for name in self.info_columns],
                axis=-1)

        # Slicing doesn't preserve static shape information, so set the shapes
        # manually. This way the `tf.data.Datasets` are easier to inspect.
        inputs.set_shape([None, self.input_width, None])
        labels.set_shape([None, self.label_width, None])
        infos.set_shape([None, self.info_width, None])

        return inputs, labels, infos

    def plot(self, model=None, plot_col='T (degC)', max_subplots=3):
        inputs, labels, infos = self.example
        plt.figure(figsize=(12, 8))
        plot_col_index = self.column_indices[plot_col]
        max_n = min(max_subplots, len(inputs))
        for n in range(max_n):
            plt.subplot(3, 1, n + 1)
            plt.ylabel(f'{plot_col} [normed]')
            plt.plot(self.input_indices, inputs[n, :, plot_col_index],
                     label='Inputs', marker='.', zorder=-10)

            if self.label_columns:
                label_col_index = self.label_columns_indices.get(plot_col, None)
            else:
                label_col_index = plot_col_index

            if label_col_index is None:
                continue

            plt.scatter(self.label_indices, labels[n, :, label_col_index],
                        edgecolors='k', label='Labels', c='#2ca02c', s=64)
            if model is not None:
                predictions = model(inputs)
                plt.scatter(self.label_indices, predictions[n, :, label_col_index],
                            marker='X', edgecolors='k', label='Predictions',
                            c='#ff7f0e', s=64)

            if n == 0:
                plt.legend()

    def make_dataset(self, data):
        data = np.array(data, dtype=np.float32)
        ds = tf.keras.preprocessing.timeseries_dataset_from_array(
            data=data,
            targets=None,
            sequence_length=self.total_window_size,
            sequence_stride=1,
            sampling_rate=self.sampling_rate,
            shuffle=self.shuffle,
            batch_size=self.batch_size)

        ds = ds.map(self.split_window)

        return ds

    @property
    def get_dataset(self):
        return self.make_dataset(self.dataframe)

    @property
    def example(self):
        """Get and cache an example batch of `inputs, labels` for plotting."""
        result = getattr(self, '_example', None)
        if result is None:
            print("No example batch was found!")
            # No example batch was found, so get one from the `.train` dataset
            result = next(iter(self.train))
            # And cache it for next time
            self._example = result
        return result

    @example.setter
    def example(self, example):
        print("Setting an example window")
        self._example = example
