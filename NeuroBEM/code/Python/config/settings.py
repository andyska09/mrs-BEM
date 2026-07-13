import os
import pathlib
import shutil
import sys
import time

import yaml


class Settings:
    def __init__(self, settings_yaml, generate_log=True):
        assert os.path.isfile(settings_yaml), settings_yaml

        with open(settings_yaml, 'r') as stream:
            settings = yaml.safe_load(stream)

            # --- hardware ---
            hardware = settings['hardware']
            gpu_device = hardware['gpu_device']
            assert isinstance(gpu_device, int)
            assert gpu_device >= 0
            os.environ['CUDA_DEVICE_ORDER'] = 'PCI_BUS_ID'
            os.environ['CUDA_VISIBLE_DEVICES'] = str(gpu_device)

            self.num_cpu_workers = hardware['num_cpu_workers']
            if self.num_cpu_workers < 0 and sys.version_info[0] >= 3:
                self.num_cpu_workers = os.cpu_count()
            elif sys.version_info[0] < 3:
                self.num_cpu_workers = 4
                print("Detected python 2, will use 4 workers.")
            self.max_memory = hardware['max_memory']

            self.base_type = settings['base_type']

            # --- directories ---
            directories = settings['dir']
            self.train_dir = directories['train']
            self.val_dir = directories['val']
            self.test_dir = directories['test']
            self.log_dir = directories['log']

            if self.train_dir == "":
                self.train_dir = "data/" + self.base_type + "/train"
            if self.val_dir == "":
                self.val_dir = "data/" + self.base_type + "/validation"
            if self.test_dir == "":
                self.test_dir = "data/" + self.base_type + "/test"

            assert os.path.isdir(self.train_dir)
            assert os.path.isdir(self.val_dir)
            assert os.path.isdir(self.test_dir)

            timestr = time.strftime("%Y%m%d-%H%M%S")
            self.log_dir = os.path.join(self.log_dir, timestr)

            if generate_log:
                os.makedirs(self.log_dir)
                # copy the settings file
                settings_copy_filepath = os.path.join(self.log_dir, 'settings.yaml')
                shutil.copyfile(settings_yaml, settings_copy_filepath)
                # copy the network file
                net_copy_filepath = os.path.join(self.log_dir, 'nets.py')
                root_path = pathlib.Path().absolute()
                net_file = os.path.join(root_path, 'utils/nets.py')
                assert os.path.isfile(net_file)
                shutil.copy(net_file, net_copy_filepath)
            self.ckpt_dir = os.path.join(self.log_dir, 'checkpoints')
            if generate_log:
                os.mkdir(self.ckpt_dir)
            self.vis_dir = os.path.join(self.log_dir, 'visualization')
            if generate_log:
                os.mkdir(self.vis_dir)

            # --- checkpoint ---
            checkpoint = settings['checkpoint']
            self.save_every_n_epochs = checkpoint['save_every_n_epochs']
            assert self.save_every_n_epochs > 0
            self.resume_training = checkpoint['resume_training']
            assert isinstance(self.resume_training, bool)
            self.resume_ckpt_file = checkpoint['resume_file']

            # --- quadrotor ---
            quadrotor = settings['quadrotor']
            self.quad_mass = quadrotor['mass']
            self.quad_inertia = quadrotor['inertia']

            # --- dataloading ---
            dataloading = settings['dataloading']
            self.history_len = dataloading['history_len']
            self.label_len = dataloading['label_len']
            self.label_shift = dataloading['label_shift']
            self.sampling_rate = dataloading['sampling_rate']
            self.input_use_pos = dataloading['use_pos']
            self.input_use_att = dataloading['use_att']
            self.input_use_linvel = dataloading['use_linvel']
            self.input_use_angvel = dataloading['use_angvel']
            self.input_use_motors = dataloading['use_motors']
            self.input_use_base_force = dataloading['use_base_force']
            self.input_use_base_torque = dataloading['use_base_torque']

            self.feature_dim = 0
            if self.input_use_pos:
                self.feature_dim += 3
            if self.input_use_att:
                self.feature_dim += 4
            if self.input_use_angvel:
                self.feature_dim += 3
            if self.input_use_linvel:
                self.feature_dim += 3
            if self.input_use_motors:
                self.feature_dim += 4
            if self.input_use_base_force:
                self.feature_dim += 3
            if self.input_use_base_torque:
                self.feature_dim += 3

            # --- network ---
            network = settings['network']
            self.architecture = network['architecture']

            if self.architecture == 'TCN' or self.architecture == 'TCN_ORIG':
                tcn_settings = network['tcn']
                self.tcn_layers = tcn_settings['layers']
                self.tcn_kernel_sizes = tcn_settings['kernel_sizes']
                self.tcn_filters = tcn_settings['n_filters']
                self.tcn_final_mlp_neurons = tcn_settings['final_mlp_neurons']
                # self.tcn_use_final_mlp = tcn_settings['use_final_mlp']
                self.tcn_2_head = tcn_settings['use_two_heads']

                assert len(self.tcn_filters) >= self.tcn_layers
                assert len(self.tcn_kernel_sizes) >= self.tcn_layers
                assert len(self.tcn_kernel_sizes) == len(self.tcn_filters)
                assert all(i % 2 for i in self.tcn_kernel_sizes), 'Use odd kernel sizes'

                # sanity check for history len
                output_len = self.history_len
                for i in range(self.tcn_layers):
                    if output_len < self.tcn_kernel_sizes[i]:
                        assert False, 'Kernel sizes not compatible with input history length'
                    output_len -= (self.tcn_kernel_sizes[i] - 1)
                    assert output_len >= 1, 'Kernel sizes not compatible with input history length'
            elif self.architecture == 'MLP':
                mlp_settings = network['mlp']
                self.mlp_n_neurons = mlp_settings['n_neurons']
                self.mlp_2_head = mlp_settings['use_two_heads']

            # --- loss ---
            loss = settings['loss']
            self.weight_force = loss['weight_force']
            self.weight_torque = loss['weight_torque']
            self.axis_weight_force = loss['axis_weight_force']
            self.axis_weight_torque = loss['axis_weight_torque']

            # --- optimization ---
            optimization = settings['optim']
            self.max_epochs = optimization['max_epochs']
            self.batch_size = optimization['batch_size']

            assert self.max_epochs > 0
            assert self.batch_size >= 1

            self.init_lr = float(optimization['init_lr'])
            assert self.init_lr > 0

            self.l2_regularization = float(optimization['l2_regularization'])
            assert self.l2_regularization >= 0

            scheduler = optimization['scheduler']
            self.scheduler_first_decay_steps = scheduler['first_decay_steps']
            self.scheduler_t_mul = scheduler['t_mul']
            self.scheduler_m_mul = float(scheduler['m_mul'])
            self.scheduler_alpha = scheduler['alpha']

            assert self.scheduler_first_decay_steps >= 0
            assert self.scheduler_t_mul >= 1.0
            assert self.scheduler_alpha >= 0.0
            assert 0 < self.scheduler_m_mul <= 1.0

            # --- visualization ---
            visualization = settings['visualization']
            self.generate_plots = visualization['generate_plots']
            self.window_size_viz = visualization['window_size_viz']
            print("#############################")
            print(self.generate_plots)
