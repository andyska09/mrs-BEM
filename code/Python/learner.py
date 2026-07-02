import os
from pathlib import Path

import matplotlib as mpl
import numpy as np
import tensorflow as tf
import yaml
from tqdm import tqdm

mpl.use('Agg')  # make sure plotting works in headless mode

from utils.visualization import create_plot_dict
from utils.loader import create_dataset
from utils.nets import create_network
from utils.loss import ForceLoss, TorqueLoss
from utils.normalization import Normalization


class Learner:
    def __init__(self, settings):
        self.config = settings

        physical_devices = tf.config.list_physical_devices('GPU')
        print(physical_devices)
        print(len(physical_devices))
        if len(physical_devices) > 0:
            # Restrict GPU memory allocated by TensorFlow
            try:
                tf.config.experimental.set_virtual_device_configuration(physical_devices[0], [
                    tf.config.experimental.VirtualDeviceConfiguration(memory_limit=self.config.max_memory)])
                logical_gpus = tf.config.experimental.list_logical_devices('GPU')
                print(len(physical_devices), "Physical GPUs,", len(logical_gpus), "Logical GPUs")
                print("Limiting allocated GPU memory to %d MB" % self.config.max_memory)
            except RuntimeError as e:
                # Virtual devices must be set before GPUs have been initialized
                print(e)

        self.min_val_loss = tf.Variable(np.inf, name='min_val_loss', trainable=False)

        self.normalization = None
        self.input_features = None
        self.network = create_network(self.config)

        self.forces_loss = ForceLoss(self.config.axis_weight_force)
        self.torques_loss = TorqueLoss(self.config.axis_weight_torque)

        if self.config.scheduler_first_decay_steps > 0:
            self.learning_rate_fn = tf.keras.experimental.CosineDecayRestarts(
                self.config.init_lr,
                self.config.scheduler_first_decay_steps,
                self.config.scheduler_t_mul,
                self.config.scheduler_m_mul,
                self.config.scheduler_alpha)
        else:
            self.learning_rate_fn = self.config.init_lr

        self.optimizer = tf.keras.optimizers.Adam(learning_rate=self.learning_rate_fn)

        self.train_loss = tf.keras.metrics.Mean(name='train_loss')
        self.train_forces_loss = tf.keras.metrics.Mean(name='train_forces_loss')
        self.train_torques_loss = tf.keras.metrics.Mean(name='train_torques_loss')
        self.val_loss = tf.keras.metrics.Mean(name='val_loss')
        self.val_forces_loss = tf.keras.metrics.Mean(name='val_forces_loss')
        self.val_torques_loss = tf.keras.metrics.Mean(name='val_torques_loss')
        self.val_forces_rmse = tf.keras.metrics.Mean(name='val_forces_rmse')
        self.val_torques_rmse = tf.keras.metrics.Mean(name='val_torques_rmse')
        self.val_forces_mad = tf.keras.metrics.Mean(name='val_forces_mad')
        self.val_torques_mad = tf.keras.metrics.Mean(name='val_torques_mad')

        self.global_epoch = tf.Variable(0)
        self.ckpt = tf.train.Checkpoint(step=self.global_epoch,
                                        optimizer=self.optimizer,
                                        net=self.network)

        if self.config.resume_training:
            if self.ckpt.restore(self.config.resume_ckpt_file):
                print("------------------------------------------")
                print("Restored from {}".format(self.config.resume_ckpt_file))
                print("------------------------------------------")
                return

        print("------------------------------------------")
        print("Initializing network from scratch.")
        print("------------------------------------------")

    @tf.function
    def train_step(self, inputs, labels):
        '''

        :param inputs:
        :param labels:
        :return:
        '''
        with tf.GradientTape() as tape:
            predictions = self.network(inputs)
            forces_loss = self.forces_loss(labels, predictions)
            torques_loss = self.torques_loss(labels, predictions)
            loss = self.config.weight_force * forces_loss + self.config.weight_torque * torques_loss
        gradients = tape.gradient(loss, self.network.trainable_variables)
        # gradients = [tf.clip_by_norm(g, 1) for g in gradients]  # potential gradient clipping
        self.optimizer.apply_gradients(zip(gradients, self.network.trainable_variables))
        self.train_loss.update_state(loss)
        self.train_forces_loss.update_state(forces_loss)
        self.train_torques_loss.update_state(torques_loss)
        return predictions, gradients

    @tf.function
    def val_step(self, inputs, labels):
        '''

        :param inputs:
        :param labels:
        :return:
        '''

        predictions = self.network(inputs)

        forces_loss = self.forces_loss(labels, predictions)
        torques_loss = self.torques_loss(labels, predictions)
        loss = self.config.weight_force * forces_loss + self.config.weight_torque * torques_loss

        self.val_loss.update_state(loss)
        self.val_forces_loss.update_state(forces_loss)
        self.val_torques_loss.update_state(torques_loss)

        return predictions

    @tf.function
    def test_step(self, inputs):
        '''

        :param inputs:
        :return:
        '''
        predictions = self.network(inputs)
        return predictions

    def write_train_summaries(self, features, gradients):
        '''

        :param features:
        :param gradients:
        :return:
        '''
        with self.summary_writer.as_default():
            tf.summary.scalar('Train/Total_Loss', self.train_loss.result(),
                              step=self.optimizer.iterations)
            tf.summary.scalar('Train/Forces_Loss', self.train_forces_loss.result(),
                              step=self.optimizer.iterations)
            tf.summary.scalar('Train/Torques_Loss', self.train_torques_loss.result(),
                              step=self.optimizer.iterations)
            if self.config.scheduler_first_decay_steps > 0:
                tf.summary.scalar('Train/Learning_Rate', self.optimizer.lr(self.optimizer.iterations),
                                  step=self.optimizer.iterations)
            for g, v in zip(gradients, self.network.trainable_variables):
                tf.summary.histogram(v.name, g, step=self.optimizer.iterations)

    def train(self):
        '''

        :return:
        '''
        print("Training Network")
        self.summary_writer = tf.summary.create_file_writer(self.config.log_dir)
        self.ckpt_manager = tf.train.CheckpointManager(self.ckpt, self.config.ckpt_dir, max_to_keep=20)

        # Create a dataset from all files in a given directory
        dataset_train = create_dataset(self.config.train_dir, self.config, training=True)
        self.input_features = dataset_train.input_features
        # Normalization coeffs are computed for every dataset, we use the ones from the training set
        self.normalization = Normalization(dataset_train.get_normalization())
        dataset_val = create_dataset(self.config.val_dir, self.config, training=False)
        # write the normalization coefficients to disk
        self.write_summary_to_disk(self.normalization.get_coeffs())

        save_every_n = max(1, int(len(dataset_train.data) / 100))
        for epoch in range(self.config.max_epochs):
            # Train
            for k, (features, labels, infos) in enumerate(tqdm(dataset_train.data)):
                features = self.normalization.normalize_inputs(features)
                labels = self.normalization.normalize_labels(labels)
                predictions, gradients = self.train_step(features, labels)
                if tf.equal(k % save_every_n, 0):
                    self.write_train_summaries(features, gradients)
                    self.train_loss.reset_states()
                    self.train_forces_loss.reset_states()
                    self.train_torques_loss.reset_states()
                    # features and labels are scaled back to physical scale
                    labels = self.normalization.unnormalize_labels(labels)
                    predictions = self.normalization.unnormalize_labels(predictions)
                    labels = np.squeeze(labels.numpy())
                    predictions = np.squeeze(predictions.numpy())

                    # compute the actual forces and torques errors
                    RMSE_force = np.sqrt(((predictions[:, :3] - labels[:, :3]) ** 2).mean())
                    RMSE_torque = np.sqrt(((predictions[:, 3:] - labels[:, 3:]) ** 2).mean())
                    MAD_force = np.abs(predictions[:, :3] - labels[:, :3] - (
                            predictions[:, :3] - labels[:, :3]).mean()).mean()
                    MAD_torque = np.abs(predictions[:, 3:] - labels[:, 3:] - (
                            predictions[:, 3:] - labels[:, 3:]).mean()).mean()
                    with self.summary_writer.as_default():
                        tf.summary.scalar("Train/Force_RMSE", RMSE_force,
                                          step=self.optimizer.iterations)
                        tf.summary.scalar("Train/Torque_RMSE", RMSE_torque,
                                          step=self.optimizer.iterations)
                        tf.summary.scalar("Train/Force_MAD", MAD_force,
                                          step=self.optimizer.iterations)
                        tf.summary.scalar("Train/Torque_MAD", MAD_torque,
                                          step=self.optimizer.iterations)

            # Eval
            val_labels = []
            val_predictions = []
            val_infos = []
            for k, (features, labels, infos) in enumerate(tqdm(dataset_val.data)):
                # features and labels are normalized before passing to the network
                features = self.normalization.normalize_inputs(features)
                labels = self.normalization.normalize_labels(labels)
                prediction = self.val_step(features, labels)

                # features and labels are scaled back to physical scale
                labels = self.normalization.unnormalize_labels(labels)
                prediction = self.normalization.unnormalize_labels(prediction)

                val_labels.append(np.squeeze(labels.numpy()))
                val_predictions.append(np.squeeze(prediction.numpy()))
                val_infos.append(np.squeeze(infos.numpy()[:, -1, :]))

            # plot the result
            val_labels_np = np.vstack(val_labels)
            val_predictions_np = np.vstack(val_predictions)
            val_infos_np = np.vstack(val_infos)

            # compute the actual forces and torques errors
            RMSE_force = np.sqrt(((val_predictions_np[:, :3] - val_labels_np[:, :3]) ** 2).mean())
            RMSE_torque = np.sqrt(((val_predictions_np[:, 3:] - val_labels_np[:, 3:]) ** 2).mean())
            MAD_force = np.abs(val_predictions_np[:, :3] - val_labels_np[:, :3] - (
                    val_predictions_np[:, :3] - val_labels_np[:, :3]).mean()).mean()
            MAD_torque = np.abs(val_predictions_np[:, 3:] - val_labels_np[:, 3:] - (
                    val_predictions_np[:, 3:] - val_labels_np[:, 3:]).mean()).mean()

            if self.config.generate_plots:
                # for visualization, we only plot a sequence of length N, randomly drawn from the validation data
                window_size_viz = min(self.config.window_size_viz, val_labels_np.shape[0] - 1)
                window_shift = np.random.randint(0, val_labels_np.shape[0] - window_size_viz)
                plot_dict = create_plot_dict(val_predictions_np[window_shift:window_shift + window_size_viz, :],
                                             val_labels_np[window_shift:window_shift + window_size_viz, :],
                                             val_infos_np[window_shift:window_shift + window_size_viz, :])

            validation_loss = self.val_loss.result()
            with self.summary_writer.as_default():
                tf.summary.scalar("Validation/Total_Loss", self.val_loss.result(),
                                  step=tf.cast(self.global_epoch, dtype=tf.int64))
                tf.summary.scalar("Validation/Forces_Loss", self.val_forces_loss.result(),
                                  step=tf.cast(self.global_epoch, dtype=tf.int64))
                tf.summary.scalar("Validation/Torques_Loss", self.val_torques_loss.result(),
                                  step=tf.cast(self.global_epoch, dtype=tf.int64))
                tf.summary.scalar("Validation/Force_RMSE", RMSE_force,
                                  step=tf.cast(self.global_epoch, dtype=tf.int64))
                tf.summary.scalar("Validation/Torque_RMSE", RMSE_torque,
                                  step=tf.cast(self.global_epoch, dtype=tf.int64))
                tf.summary.scalar("Validation/Force_MAD", MAD_force,
                                  step=tf.cast(self.global_epoch, dtype=tf.int64))
                tf.summary.scalar("Validation/Torque_MAD", MAD_torque,
                                  step=tf.cast(self.global_epoch, dtype=tf.int64))
                if self.config.generate_plots:
                    # plot cumulative forces/torques
                    tf.summary.image("Cumulative/FX", plot_dict['Tot. Force x [N]'],
                                     step=tf.cast(self.global_epoch, dtype=tf.int64))
                    tf.summary.image("Cumulative/FY", plot_dict['Tot. Force y [N]'],
                                     step=tf.cast(self.global_epoch, dtype=tf.int64))
                    tf.summary.image("Cumulative/FZ", plot_dict['Tot. Force z [N]'],
                                     step=tf.cast(self.global_epoch, dtype=tf.int64))
                    tf.summary.image("Cumulative/TX", plot_dict['Tot. Torque x [Nm]'],
                                     step=tf.cast(self.global_epoch, dtype=tf.int64))
                    tf.summary.image("Cumulative/TY", plot_dict['Tot. Torque y [Nm]'],
                                     step=tf.cast(self.global_epoch, dtype=tf.int64))
                    tf.summary.image("Cumulative/TZ", plot_dict['Tot. Torque z [Nm]'],
                                     step=tf.cast(self.global_epoch, dtype=tf.int64))
                    # plot residual forces/torques
                    tf.summary.image("Residual/FX", plot_dict['Res. Force x [N]'],
                                     step=tf.cast(self.global_epoch, dtype=tf.int64))
                    tf.summary.image("Residual/FY", plot_dict['Res. Force y [N]'],
                                     step=tf.cast(self.global_epoch, dtype=tf.int64))
                    tf.summary.image("Residual/FZ", plot_dict['Res. Force z [N]'],
                                     step=tf.cast(self.global_epoch, dtype=tf.int64))
                    tf.summary.image("Residual/TX", plot_dict['Res. Torque x [Nm]'],
                                     step=tf.cast(self.global_epoch, dtype=tf.int64))
                    tf.summary.image("Residual/TY", plot_dict['Res. Torque y [Nm]'],
                                     step=tf.cast(self.global_epoch, dtype=tf.int64))
                    tf.summary.image("Residual/TZ", plot_dict['Res. Torque z [Nm]'],
                                     step=tf.cast(self.global_epoch, dtype=tf.int64))

            self.val_loss.reset_states()
            self.val_forces_loss.reset_states()
            self.val_torques_loss.reset_states()

            self.global_epoch = self.global_epoch + 1
            self.ckpt.step.assign_add(1)

            print("Epoch: {:2d}, Validation Loss: {:.4f}".format(self.global_epoch, validation_loss))

            if validation_loss < self.min_val_loss or ((epoch + 1) % self.config.save_every_n_epochs) == 0:
                if validation_loss < self.min_val_loss:
                    self.min_val_loss = validation_loss
                    # the best model is continuously saved to .pb format
                    fname_pb_model = os.path.join(self.config.log_dir, "best_model")
                    tf.saved_model.save(self.network, fname_pb_model)
                save_path = self.ckpt_manager.save()
                print("Saved checkpoint for epoch {}: {}".format(int(self.ckpt.step), save_path))

        print("------------------------------")
        print("Training finished successfully")
        print("------------------------------")

    def test(self):
        '''
        High-level test function.
        :return:
        '''
        print("Testing Network")
        dataset_test = create_dataset(self.config.test_dir,
                                      self.config, training=False)

        # get normalization coefficients
        fname_yaml = os.path.join(Path(self.config.resume_ckpt_file).parent.parent, "all.yaml")
        with open(fname_yaml, 'r') as f:
            yaml_list = yaml.load(f, Loader=yaml.SafeLoader)
            self.normalization = Normalization([np.array(yaml_list['means_in']), np.array(yaml_list['stds_in']),
                                                np.array(yaml_list['means_out']), np.array(yaml_list['stds_out'])])

        for k, (features, labels) in enumerate(tqdm(dataset_test.data)):
            # features are normalized before passing to the network
            features = self.normalization.normalize_inputs(features)
            prediction = self.test_step(features)
            # features are scaled back to physical scale

            # we could do something with this prediction...
            prediction = self.normalization.unnormalize_labels(prediction)

        # Sanity-check
        input_tensor_np = np.ones((1, self.config.history_len, self.config.feature_dim))
        input_tensor = tf.constant(input_tensor_np, dtype=tf.float32)
        print(self.test_step(input_tensor))

        print("------------------------------")
        print("Testing finished successfully")
        print("------------------------------")

    def write_summary_to_disk(self, normalization_coeffs):
        '''

        :param normalization_coeffs:
        :return:
        '''
        yaml_dict = {}
        yaml_dict["filename"] = ""  # TODO: fill in later
        yaml_dict["history"] = self.config.history_len
        yaml_dict["input_names"] = self.input_features
        yaml_dict["output_names"] = ["residual_fx",
                                     "residual_fy",
                                     "residual_fz",
                                     "residual_tx",
                                     "residual_ty",
                                     "residual_tz"]
        yaml_dict["num_input"] = len(yaml_dict["input_names"])
        yaml_dict["num_output"] = len(yaml_dict["output_names"])
        (means_in, stds_in, means_out, stds_out) = normalization_coeffs
        yaml_dict["means_in"] = means_in.tolist()
        yaml_dict["stds_in"] = stds_in.tolist()
        yaml_dict["mass_train"] = self.config.quad_mass
        yaml_dict["inertia_train"] = self.config.quad_inertia
        yaml_dict["means_out"] = means_out.tolist()
        yaml_dict["stds_out"] = stds_out.tolist()

        fname_yaml = os.path.join(self.config.log_dir, "all.yaml")
        with open(fname_yaml, 'w') as f:
            s = yaml.dump(yaml_dict, f, default_flow_style=False)
