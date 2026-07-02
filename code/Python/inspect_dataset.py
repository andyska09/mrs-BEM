import argparse
import os

import matplotlib.pyplot as plt
import numpy as np
import pandas as pd
import pyquaternion


def plot_data(df, x_cols, y_cols):
    assert len(x_cols) == len(y_cols) or len(x_cols) == 1, 'Invalid plotting columns'

    fig, ax = plt.subplots(figsize=(12, 7))
    if len(x_cols) == 1:
        for y_col in y_cols:
            ax.plot(df[x_cols[0]].values, df[y_col].values, label=y_col)
    else:
        for i in range(len(x_cols)):
            ax.plot(df[x_cols[i]].values, df[y_cols[i]].values, label=y_cols[i])

    ax.legend(loc='upper right')
    ax.grid()

    plt.show()


def main():
    parser = argparse.ArgumentParser(description='Inspect datafile.')
    parser.add_argument('--data_file', help='File to load data', required=True)

    args = parser.parse_args()
    data_file = args.data_file

    assert os.path.isfile(data_file), "Not Found data file"

    # Load dataframe
    header = ["t", "ang_acc_x", "ang_acc_y", "ang_acc_z", \
              "ang_vel_x", "ang_vel_y", "ang_vel_z", \
              "quat_x", "quat_y", "quat_z", "quat_w", \
              "acc_x", "acc_y", "acc_z", \
              "vel_x", "vel_y", "vel_z", \
              "pos_x", "pos_y", "pos_z", \
              "mot_1", "mot_2", "mot_3", "mot_4", \
              "dmot_1", "dmot_2", "dmot_3", "dmot_4", "vbat", \
              "model_fx", "model_fy", "model_fz", \
              "model_tx", "model_ty", "model_tz", \
              "residual_fx", "residual_fy", "residual_fz", \
              "residual_tx", "residual_ty", "residual_tz"]

    print("Decoding [%s]" % data_file)
    df = pd.read_csv(data_file, delimiter=',', names=header)

    df['gt_fx'] = df['model_fx'] + df['residual_fx']
    df['gt_fy'] = df['model_fy'] + df['residual_fy']
    df['gt_fz'] = df['model_fz'] + df['residual_fz']
    df['gt_tx'] = df['model_tx'] + df['residual_tx']
    df['gt_ty'] = df['model_ty'] + df['residual_ty']
    df['gt_tz'] = df['model_tz'] + df['residual_tz']

    ################################
    # check 1: make the derivatives actually sense? (some wrong frame convention?, do numeric derivatives match?)
    ################################
    # iterate over data, compute numeric bodyrates
    numeric_bodyrates_list = []
    for i in range(1, len(df)):
        t_curr = df['t'].values[i]
        t_prev = df['t'].values[i - 1]
        q_curr = pyquaternion.Quaternion(df['quat_w'].values[i],
                                         df['quat_x'].values[i],
                                         df['quat_y'].values[i],
                                         df['quat_z'].values[i]).normalised
        q_prev = pyquaternion.Quaternion(df['quat_w'].values[i - 1],
                                         df['quat_x'].values[i - 1],
                                         df['quat_y'].values[i - 1],
                                         df['quat_z'].values[i - 1]).normalised
        q_omega_body = q_prev.conjugate * q_curr
        omega_body = 2.0 / (t_curr - t_prev) * np.array([q_omega_body.x,
                                                         q_omega_body.y,
                                                         q_omega_body.z])
        numeric_bodyrates_list.append(omega_body)

    numeric_bodyrates = np.array(numeric_bodyrates_list)
    df['numeric_ang_vel_x'] = np.concatenate([numeric_bodyrates[:, 0], np.zeros(1)])
    df['numeric_ang_vel_y'] = np.concatenate([numeric_bodyrates[:, 1], np.zeros(1)])
    df['numeric_ang_vel_z'] = np.concatenate([numeric_bodyrates[:, 2], np.zeros(1)])

    df['numeric_ang_acc_x'] = np.concatenate([np.diff(df['ang_vel_x']) / np.diff(df['t']), np.zeros(1)])
    df['numeric_ang_acc_y'] = np.concatenate([np.diff(df['ang_vel_y']) / np.diff(df['t']), np.zeros(1)])
    df['numeric_ang_acc_z'] = np.concatenate([np.diff(df['ang_vel_z']) / np.diff(df['t']), np.zeros(1)])

    plot_data(df, ['t'], ['ang_acc_x', 'numeric_ang_acc_x'])
    # plot_data(df, ['t'], ['ang_vel_x', 'numeric_ang_vel_x'])

    ################################
    # check 2: do motor differences translate into measured moments?
    ################################
    # what is the motor numbering?
    # -> assuming: 4     1
    #                 x
    #              3     2
    # without fancy scaling, just to look at the shape of the 'pseudo-torque' curve
    random_scale = 0.05
    df['motor_torque_x'] = (-df['mot_1'] - df['mot_2'] + df['mot_3'] + df['mot_4']) * random_scale

    # plot_data(df, ['t'], ['ang_acc_x', 'numeric_ang_acc_x', 'motor_torque_x'])


if __name__ == "__main__":
    main()
