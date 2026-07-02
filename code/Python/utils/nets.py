import tensorflow as tf
from tensorflow.keras import regularizers
from tensorflow.keras.layers import Dense, LeakyReLU, Conv1D, Reshape, LSTM
from tensorflow.keras.layers import Flatten


def create_network(settings):
    net = None

    # Free up RAM in case the model definition cells were run multiple times
    tf.keras.backend.clear_session()

    if settings.architecture == "MLP":
        net = get_mlp_model(settings)
    elif settings.architecture == "TCN":
        net = get_tcn_model(settings)
    elif settings.architecture == "TCN_ORIG":
        net = get_orig_tcn_model(settings)
    elif settings.architecture == "RNN":
        net = get_rnn_model(settings)

    net.summary()

    return net


def get_orig_tcn_model(settings):
    inputs = tf.keras.Input(shape=(settings.history_len, settings.feature_dim))

    x = inputs
    for i in range(settings.tcn_layers):
        x = Conv1D(filters=settings.tcn_filters[i], kernel_size=(settings.tcn_kernel_sizes[i]), activation=None,
                   kernel_regularizer=regularizers.l2(settings.l2_regularization))(x)
        x = LeakyReLU(alpha=0.2)(x)

    if settings.tcn_2_head:
        # split feature map into two separate heads
        force_feature = Flatten()(x)
        force_feature = Dense(20, activation=None, use_bias=True,
                              kernel_regularizer=regularizers.l2(settings.l2_regularization))(force_feature)
        force_feature = LeakyReLU(alpha=0.2)(force_feature)
        force_feature = Dense(3, activation=None, use_bias=True,
                              kernel_regularizer=regularizers.l2(settings.l2_regularization))(force_feature)

        torque_feature = Flatten()(x)
        torque_feature = Dense(20, activation=None, use_bias=True,
                               kernel_regularizer=regularizers.l2(settings.l2_regularization))(torque_feature)
        torque_feature = LeakyReLU(alpha=0.2)(torque_feature)
        torque_feature = Dense(3, activation=None, use_bias=True,
                               kernel_regularizer=regularizers.l2(settings.l2_regularization))(torque_feature)

        # concatenate both heads again
        outputs = tf.concat([force_feature, torque_feature], 1)
        outputs = tf.expand_dims(outputs, axis=1)
        model = tf.keras.Model(inputs, outputs)
        return model

    # elif settings.tcn_use_final_mlp:
    #     x = Flatten()(x)
    #     x = Dense(20, activation=None, use_bias=True, kernel_regularizer=regularizers.l2(settings.l2_regularization))(x)
    #     x = LeakyReLU(alpha=0.2)(x)
    #     x = Dense(6, activation=None, use_bias=True, kernel_regularizer=regularizers.l2(settings.l2_regularization))(x)
    else:
        for i in range(settings.tcn_layers):
            x = Conv1D(filters=settings.tcn_filters[i], kernel_size=(settings.tcn_kernel_sizes[i]), activation=None,
                       kernel_regularizer=regularizers.l2(settings.l2_regularization))(x)
            x = LeakyReLU(alpha=0.2)(x)
        x = x[:, -1, :]  # only return the last prediction

    x = tf.expand_dims(x, axis=1)
    outputs = x

    # Define the model
    model = tf.keras.Model(inputs, outputs)
    return model


def get_tcn_model(settings):
    inputs = tf.keras.Input(shape=(settings.history_len, settings.feature_dim))
    x = inputs

    if settings.tcn_2_head:
        # split feature map into two separate heads
        force_feature = x
        torque_feature = x
        for i in range(settings.tcn_layers):
            force_feature = Conv1D(filters=settings.tcn_filters[i], kernel_size=(settings.tcn_kernel_sizes[i]),
                                   activation=None,
                                   kernel_regularizer=regularizers.l2(settings.l2_regularization))(force_feature)
            force_feature = LeakyReLU(alpha=0.2)(force_feature)
            torque_feature = Conv1D(filters=settings.tcn_filters[i], kernel_size=(settings.tcn_kernel_sizes[i]),
                                    activation=None,
                                    kernel_regularizer=regularizers.l2(settings.l2_regularization))(torque_feature)
            torque_feature = LeakyReLU(alpha=0.2)(torque_feature)

        force_feature = Flatten()(force_feature)
        torque_feature = Flatten()(torque_feature)
        for layer_size in settings.tcn_final_mlp_neurons:
            force_feature = Dense(layer_size, activation=None, use_bias=True,
                                  kernel_regularizer=regularizers.l2(settings.l2_regularization))(force_feature)
            force_feature = LeakyReLU(alpha=0.2)(force_feature)
            torque_feature = Dense(layer_size, activation=None, use_bias=True,
                                   kernel_regularizer=regularizers.l2(settings.l2_regularization))(torque_feature)
            torque_feature = LeakyReLU(alpha=0.2)(torque_feature)

        force_feature = Dense(3, activation=None, use_bias=True,
                              kernel_regularizer=regularizers.l2(settings.l2_regularization))(force_feature)
        torque_feature = Dense(3, activation=None, use_bias=True,
                               kernel_regularizer=regularizers.l2(settings.l2_regularization))(torque_feature)

        # concatenate both heads again
        outputs = tf.concat([force_feature, torque_feature], 1)
        outputs = tf.expand_dims(outputs, axis=1)
        model = tf.keras.Model(inputs, outputs)
        return model

    # elif settings.tcn_use_final_mlp:
    #     x = Flatten()(x)
    #     x = Dense(20, activation=None, use_bias=True, kernel_regularizer=regularizers.l2(settings.l2_regularization))(x)
    #     x = LeakyReLU(alpha=0.2)(x)
    #     x = Dense(6, activation=None, use_bias=True, kernel_regularizer=regularizers.l2(settings.l2_regularization))(x)
    else:
        for i in range(settings.tcn_layers):
            x = Conv1D(filters=settings.tcn_filters[i], kernel_size=(settings.tcn_kernel_sizes[i]), activation=None,
                       kernel_regularizer=regularizers.l2(settings.l2_regularization))(x)
            x = LeakyReLU(alpha=0.2)(x)
        x = x[:, -1, :]  # only return the last prediction

    x = tf.expand_dims(x, axis=1)
    outputs = x

    # Define the model
    model = tf.keras.Model(inputs, outputs)
    return model


def get_mlp_model(settings):
    inputs = tf.keras.Input(shape=(settings.history_len, settings.feature_dim))

    x = inputs
    x = Flatten()(x)

    if settings.mlp_2_head:
        # split feature map into two separate heads
        force_feature = Flatten()(x)
        torque_feature = Flatten()(x)
        for layer_size in settings.mlp_n_neurons:
            force_feature = Dense(layer_size, activation=None, use_bias=True,
                                  kernel_regularizer=regularizers.l2(settings.l2_regularization))(force_feature)
            force_feature = LeakyReLU(alpha=0.2)(force_feature)
            torque_feature = Dense(layer_size, activation=None, use_bias=True,
                                   kernel_regularizer=regularizers.l2(settings.l2_regularization))(torque_feature)
            torque_feature = LeakyReLU(alpha=0.2)(torque_feature)

        force_feature = Dense(3, activation=None, use_bias=True,
                              kernel_regularizer=regularizers.l2(settings.l2_regularization))(force_feature)
        torque_feature = Dense(3, activation=None, use_bias=True,
                               kernel_regularizer=regularizers.l2(settings.l2_regularization))(torque_feature)

        # concatenate both heads again
        outputs = tf.concat([force_feature, torque_feature], 1)
        outputs = tf.expand_dims(outputs, axis=1)
        model = tf.keras.Model(inputs, outputs)
        return model
    else:
        for layer_size in settings.mlp_n_neurons:
            x = Dense(layer_size, activation=None, use_bias=True,
                      kernel_regularizer=regularizers.l2(settings.l2_regularization))(x)
            x = LeakyReLU(alpha=0.2)(x)
        x = Dense(6, activation=None, use_bias=True, kernel_regularizer=regularizers.l2(settings.l2_regularization))(x)
        x = Reshape([1, -1])(x)

        outputs = x

        # Define the model
        model = tf.keras.Model(inputs, outputs)
        return model


def get_rnn_model(settings):
    inputs = tf.keras.Input(shape=(settings.history_len, settings.feature_dim))

    x = inputs
    x = LSTM(12, return_sequences=False)(x)
    x = LeakyReLU(alpha=0.2)(x)
    x = Dense(20, activation=None, use_bias=True)(x)
    x = LeakyReLU(alpha=0.2)(x)
    x = Dense(6, activation=None, use_bias=True)(x)
    x = Reshape([1, -1])(x)  # Adds back the time dimension.

    outputs = x

    # Define the model
    model = tf.keras.Model(inputs, outputs)
    return model
