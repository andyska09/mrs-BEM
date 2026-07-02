import tensorflow as tf
from tensorflow.keras.losses import Loss


class ForceLoss(Loss):
    def __init__(self, force_axis_weights):
        super(ForceLoss, self).__init__(reduction=tf.keras.losses.Reduction.NONE)
        self.force_axis_weights = force_axis_weights

    @tf.function
    def call(self, label, prediction):
        '''
        Computes the loss between the predicted and GT force.
        :param label:
        :param prediction:
        :return:
        '''
        # force_loss = tf.keras.losses.MSE(tf.slice(label, [0, 0, 0], [-1, -1, 3]),
        #                                  tf.slice(prediction, [0, 0, 0], [-1, -1, 3]))
        force_loss = self.force_axis_weights[0] * tf.keras.losses.MSE(tf.slice(label, [0, 0, 0], [-1, -1, 1]),
                                                                      tf.slice(prediction, [0, 0, 0], [-1, -1, 1])) + \
                     self.force_axis_weights[1] * tf.keras.losses.MSE(tf.slice(label, [0, 0, 1], [-1, -1, 1]),
                                                                      tf.slice(prediction, [0, 0, 1], [-1, -1, 1])) + \
                     self.force_axis_weights[2] * tf.keras.losses.MSE(tf.slice(label, [0, 0, 2], [-1, -1, 1]),
                                                                      tf.slice(prediction, [0, 0, 2], [-1, -1, 1]))

        return force_loss


class TorqueLoss(Loss):
    def __init__(self, torque_axis_weights):
        super(TorqueLoss, self).__init__(reduction=tf.keras.losses.Reduction.NONE)
        self.torque_axis_weights = torque_axis_weights

    @tf.function
    def call(self, label, prediction):
        '''
        Computes the loss between the predicted and GT torque.
        :param label:
        :param prediction:
        :return:
        '''
        # torque_loss = tf.keras.losses.MSE(tf.slice(label, [0, 0, 3], [-1, -1, 3]),
        #                                   tf.slice(prediction, [0, 0, 3], [-1, -1, 3]))

        torque_loss = self.torque_axis_weights[0] * tf.keras.losses.MSE(tf.slice(label, [0, 0, 3], [-1, -1, 1]),
                                                                        tf.slice(prediction, [0, 0, 3], [-1, -1, 1])) + \
                      self.torque_axis_weights[1] * tf.keras.losses.MSE(tf.slice(label, [0, 0, 4], [-1, -1, 1]),
                                                                        tf.slice(prediction, [0, 0, 4], [-1, -1, 1])) + \
                      self.torque_axis_weights[2] * tf.keras.losses.MSE(tf.slice(label, [0, 0, 5], [-1, -1, 1]),
                                                                        tf.slice(prediction, [0, 0, 5], [-1, -1, 1]))

        return torque_loss
