import io

import matplotlib as mpl
import matplotlib.pyplot as plt
import tensorflow as tf

mpl.use('Agg')  # make sure plotting works in headless mode


def create_plot_dict(predictions, labels, infos, plot_cumulative=True):
    plot_dict = {}
    y_axis_labels = ['Residual Force [N]',
                     'Residual Force [N]',
                     'Residual Force [N]',
                     'Residual Torque [Nm]',
                     'Residual Torque [Nm]',
                     'Residual Torque [Nm]']
    plot_labels = ['Res. Force x [N]', 'Res. Force y [N]', 'Res. Force z [N]',
                   'Res. Torque x [Nm]', 'Res. Torque y [Nm]', 'Res. Torque z [Nm]']
    # infos = infos[:, -1, :]
    for i in range(6):
        fig, ax = plt.subplots(figsize=(12, 7))
        ax.plot(infos[:, 0], predictions[:, i], label='Prediction')
        ax.plot(infos[:, 0], labels[:, i], label='Label')
        ax.set_title(plot_labels[i])
        ax.set_xlabel('Time [s]')
        ax.set_ylabel(y_axis_labels[i])
        ax.legend(loc='upper right')
        ax.grid()

        buf = io.BytesIO()
        fig.savefig(buf, format='png')
        buf.seek(0)
        # Convert PNG buffer to TF image
        plot_image = tf.image.decode_png(buf.getvalue(), channels=4)

        # Add the batch dimension
        plot_data = tf.expand_dims(plot_image, 0)
        plot_dict[plot_labels[i]] = plot_data
        plt.close()

    # Plot not only the residuals, but also the cumulative forces/torques
    if plot_cumulative:
        y_axis_labels = ['Total Force [N]',
                         'Total Force [N]',
                         'Total Force [N]',
                         'Total Torque [Nm]',
                         'Total Torque [Nm]',
                         'Total Torque [Nm]']
        plot_labels = ['Tot. Force x [N]', 'Tot. Force y [N]', 'Tot. Force z [N]',
                       'Tot. Torque x [Nm]', 'Tot. Torque y [Nm]', 'Tot. Torque z [Nm]']
        for i in range(6):
            fig, ax = plt.subplots(figsize=(12, 7))
            ax.plot(infos[:, 0], predictions[:, i] + infos[:, 2 + i], label='Prediction')
            ax.plot(infos[:, 0], labels[:, i] + infos[:, 2 + i], label='Ground Truth')
            ax.plot(infos[:, 0], infos[:, 2 + i], label='Base Model')
            ax.set_title(plot_labels[i])
            ax.set_xlabel('Time [s]')
            ax.set_ylabel(y_axis_labels[i])
            ax.legend(loc='upper right')
            ax.grid()

            buf = io.BytesIO()
            fig.savefig(buf, format='png')
            buf.seek(0)
            # Convert PNG buffer to TF image
            plot_image = tf.image.decode_png(buf.getvalue(), channels=4)

            # Add the batch dimension
            plot_data = tf.expand_dims(plot_image, 0)
            plot_dict[plot_labels[i]] = plot_data
            plt.close()

    return plot_dict


def plot_window(features, labels, infos):
    print('Plotting window')
    print(features.shape)
    print(labels.shape)
    print(infos.shape)
    print(infos[0, :, 0])
    fig, axs = plt.subplots(2, 3, figsize=(12, 7))
    axs[0, 0].plot(infos[0, :, 0], features[0, :, 14], label='Base Model')
    axs[0, 0].plot(infos[0, :, 0], features[0, :, 14] + infos[0, :, 8], label='Ground Truth')
    axs[0, 0].set_title("Force X")
    axs[0, 0].legend(loc='upper right')
    axs[0, 0].grid()

    axs[0, 1].plot(infos[0, :, 0], features[0, :, 15], label='Base Model')
    axs[0, 1].plot(infos[0, :, 0], features[0, :, 15] + infos[0, :, 9], label='Ground Truth')
    axs[0, 1].set_title("Force Y")
    axs[0, 1].legend(loc='upper right')
    axs[0, 1].grid()

    axs[0, 2].plot(infos[0, :, 0], features[0, :, 16], label='Base Model')
    axs[0, 2].plot(infos[0, :, 0], features[0, :, 16] + infos[0, :, 10], label='Ground Truth')
    axs[0, 2].set_title("Force Z")
    axs[0, 2].legend(loc='upper right')
    axs[0, 2].grid()

    # Torques
    axs[1, 0].plot(infos[0, :, 0], features[0, :, 17], label='Base Model')
    axs[1, 0].plot(infos[0, :, 0], features[0, :, 17] + infos[0, :, 11], label='Ground Truth')
    axs[1, 0].set_title("Torque X")
    axs[1, 0].set_xlabel('Time [s]')
    axs[1, 0].legend(loc='upper right')
    axs[1, 0].grid()

    axs[1, 1].plot(infos[0, :, 0], features[0, :, 18], label='Base Model')
    axs[1, 1].plot(infos[0, :, 0], features[0, :, 18] + infos[0, :, 12], label='Ground Truth')
    axs[1, 1].set_title("Torque Y")
    axs[1, 1].set_xlabel('Time [s]')
    axs[1, 1].legend(loc='upper right')
    axs[1, 1].grid()

    axs[1, 2].plot(infos[0, :, 0], features[0, :, 19], label='Base Model')
    axs[1, 2].plot(infos[0, :, 0], features[0, :, 19] + infos[0, :, 13], label='Ground Truth')
    axs[1, 2].set_title("Torque Z")
    axs[1, 2].set_xlabel('Time [s]')
    axs[1, 2].legend(loc='upper right')
    axs[1, 2].grid()

    plt.show()
