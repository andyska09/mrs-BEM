> **This is a partial copy of the original NeuroBEM release.**
> Source: https://download.ifi.uzh.ch/rpg/NeuroBEM/ (framework + dataset).
> The MATLAB pipeline (`Matlab/`, `Maple/`) and `ExampleData/` have been removed;
> only the parts important for the training NN and running BEM on the processed data are kept — `Python/`, `simulator/`,
> `Scripts/`. Sections below that reference the removed folders are left intact as
> documentation of the original workflow; get those files from the link above.

# Introduction
### Overview
This README gives instructions and examples how the Neuro-BEM framework can be used. Neuro-BEM can be used to accurately simulate the Dynamics of a Drone by augmenting a first-principle blade element model with a neural network.
This documents provides instructions how one obtains all required parameters and datasets to use Neuro-BEM on a new drone. **Read first from top to bottom, you might save yourself a lot of work if you know everything before hand**.

To try it out, a complete example dataset is provided under the following link: ***TODO***
The filenames are always prepended with `Ex_` to avoid confusion with your own data.

The last section, `Additional Scripts and Tools` gives an overview over the other functionalities that are not directly inline with the workflow required to use Neuro-BEM.

All MATLAB functions are documented and `help functionName` will display this help.

#### What Data is Required?
The framework consists of two parts, the BEM model and the Neural Net. For the BEM model, physical parameters of the propeller need to be identified, namely the
- propeller geometry,
- mass of the propeller,
- lift and drag coefficient of the blade,
- spring constant of the hinged-spring model.

The neural net relies purely on actual flight data on which it is trained. The data needs to include
- linear and angular acceleration measurements,
- linear and angular velocity measurements,
- position and attitude measurements,
- motor speed measurements.

The following sections detail each step of the data generation and postprocessing to ultimately arrive at Neuro-BEM model.

#### Example Dataset Installation
The example dataset is meant to be unzipped into the empty folder `/ExampleData/`. The already included .gitignore files make sure, it does not mess up the repository.

______

# Blade Element Model
### Overview
The Blade Element Model (BEM) is a first-principle model. For more information, see

> "NeuroBEM: Hybrid Aerodynamic Quadrotor Model", 2021, <cite>L. Bauersfeld, E. Kaufmann, P. Föhn, S. Sun, D. Scaramuzza</cite>
> "Helicopter performance, stability, and control", 1995, <cite>R. W. Prouty</cite>

A list of parameters that is required can be found in `bem_parameters.yaml`. The next section describes how one can obtain the numbers.

### Thrust Test Stand Measurements
#### Forces and Torques
The neccessary data can be measured using a single propeller on the thrust-test stand. A csv file of following form needs to be recorded:

`t, omega, thrust, Fx, Fy, Fz, Mx, My, Mz `

where t is the time [s], omega [rad/s] the motor speed, thrust [-] an arbitrary command, and F#, M# are the forces and moments along the corresponding axis. The thrust profile must be stepwise, with at least 2 seconds per step, i.e. command 1000PWM, wait 2s, increase to 1100PWM etc.
The motors can get hot and one may need short breaks. Tthe scripts support combination of multiple runs.

An example dataset from one thrust test stand run is provided in `ExampleData/BEM/Ex_ThrustTestStand.csv`.

All such measured datasets need to be processed. The script `Matlab/BEM/SegmentMeasurement.m` finds the segments of contant thrust and automatically extracts the mean force and torque values for the given mean omega. The result is written to a `.csv` file.

In a next step, all measurements are read in and the quadratic thrust coefficients are calculated. If the quadratic curves to not approximate the measurments *very well* something is wrong. This is done by the script `Matlab/BEM/QuadraticFit.m`. The result is written to a `.csv` file.

#### Video for coning analysis
The most difficult parameter that needs to be estimated for the BEM is the hinge-spring constant k\_beta. It can be done by analyzing the coning angle of the propeller under various speeds. In order to do this, a video of the propeller (prefferebly the propeller has an RGB color, e.g. blue). The script `Matlab/BEM/Coning.m` can be used to process the video. The rotational speed of the propeller is determined using FFT analysis from the audio of the video, which needs to be provided as a seperate file. For this reason, try to be quiet during the run. One can seperate the audio and the video with `ffmpeg` with this command:
```
$ sudo apt-get install ffmpeg
$ ffmpeg -i VideoFile.mov AudioFile.wav
```
After this is done, the script `Coning.m` can be run. Note the Warnings noted on top of the script, it is rather fragile and quite a hack. Use with uttermost care!

### BEM Parameter
#### Identification Script
After verifiying that the measurement data is good, fit the BEM model to the data generated in the prior steps. For this, use `Matlab/BEM/ParameterID.m`. To use the script *precisely* follow the instructions! The script runs an optimization that fits the lift coefficient, drag coefficient and spring constant based on the previously measured data. The values are outputted in the end.
- Set all *geometry parameters* of the propeller in `Matlab/BEM/subroutines/setParam.m`. For hints what they mean, have a look at the file `Matlab/BEM/bem_parameters.yaml`.
- Change the paths to point to the data and run the `ParameterID` script. It should stop with an error which tells you to read the docs.
- The two values, linear lift coefficient and linear drag coefficient that are printed to the screen need to be copied into the `setParam.m` file. Copy the lift value into the value for `param.a` and the drag value into `param.d`
- Comment the error line and run the script again. It will now take much longer, but now the previously calculated values are used to estimate the lift and drag on a more sophisticated propeller model.
- Once again, the newly found values need to be written to the `setParam.m` file. The lift coefficient goes into `param.cl`, the drag coefficient into `param.cd` and the spring constant into `param.kbeta`. Note: the values of cl and cd will be completely different to their linear counterparts. That's ok.
- Congratulations, you've mastered the most difficult step in this section!

#### Checking the BEM Parameter Estimate
The previously calculated numbers mean nothing to most humans. For this reason, there's a script that generates a plot which helps the user assess whether the result is correct. Execute the script `Matlab/BEM/CheckBEMApproximation.m`

The thrust and torque curves need to be fitted perfectly. If they do look worse than the simple quadratic fit performed earlier, something is wrong. The coning angle should at least be close to the datapoints, but if the curve somewhat passes through the data, this should be ok.

#### Derivation of the BEM Flapping and Coning
The flapping and coning angles are derived in a Maple Worksheet. This can be found under `Maple/BEM_Derivation.mw`. Executing and Editing requires the CAS system Maple, which is not Freeware, but ETHZ has a licence.

The worksheet is used to generate both the Matlab formulae implemented in the BEM subroutines `calc_a0.m`, `calc_a1s`, and `calc_b1s` as well as the C++ implementation. The latter one can be generated in a way similar to what is shown in Eq. 2.1 of the worksheet.

It is only neccessary to generate the equations for C++ when the parameters change since the MATLAB version is fully parametric.

#### Notes and Implementation Details
The BEM Model uses the linear drag and linear lift coefficients to calculate the flapping and coning angles because otherwise the problem would become computationally untractable. Therefore, when changing the `param.cl` or `param.cd` values, don't expect a change in the coning and flapping angles.

The MATLAB code is only meant as a tool to generate the correct values required by the C++ implementation. Therefore, it is not optimized for speed or even written cleverly.


### Deployment to C++
The values used inside the `setParam.m` script are identical to the ones that need to be supplied to the C++ implementation. Put them in the corresponding `sim_yourDroneName.yaml` file inside *agilicious*
______

# Neural Network
### Overview
The neural net is used to fit the residual torques and forces that can not be predicted by the BEM base model. To train the network, a number of real-world flights is required, where the forces and torques are known (based on linear and angular acceleration) along with the motor speeds. This data is passed to the BEM model which predicts the forces and torques. The difference between the measured values and the prediction is then fed into the neural net as training data.

### Data Generation
#### Optitrack Space
The platform needs to fly in a tracking volume, and its pose must be recorded. It is also recommended to record the commands sent by the MPC controller. Assuming the data is recorded in Dübendorf, the following command records a suitable RosBag.
```
$ rosbag record /vicon/NAME /NAME/laird_to_sbus/command
```
where `NAME` is the name of the drone that is used, e.g. blackbird.

#### Betaflight
Betaflight is used to record the motor speeds. To use this functionality, ensure that
- the ESC's on the drone have at least BLHeli\_32 Firmware Version 32.7 to support bi-directional DSHOT
- betaflight is set to DSHOT (bi-directional) motor protocol
- betaflight logging is active with the debug option RPM\_Telemetry

### Data Processing
#### Recommended File Structure
All subsequent scripts assume that all measurement data is stored in the same folder. All data from the same flight is named identically with different prefixes, suffixes and filetypes. Rename the betaflight log to match the rosbag's name, e.g. the timestamp.

It is also recommended to copy the trajectory into the same folder as the rosbag and the betaflight logs. The naming convention is `FlightID_traj.csv`.

The data folder should now contain, for each flight, the following files
- `FlightID.bag` (Rosbag)
- `FlightID.BFL` (Betaflight Log)
- `FlightID_traj.csv` (Trajectory)
The files (with ID `Ex_OptiTrack` can be again found in the `ExampleData/OptiTrack/` folder).

#### Converting the Betaflight Logs
The betaflight logs are stored in a compressed binary format and need to be extracted first. For this, download a pre-built version of the *blackbox-tools*. Instructions are found on their GitHub page.
https://github.com/betaflight/blackbox-tools
Add the folder `blackbox-tools/obj` to your path environment variable.

To convert the blackbox logs into a csv, a convenience script is provided under `Scripts/blackboxToCsv.sh`. It can be executed as
```
$ cd Scripts
$ ./blackboxToCsv.sh PATH_TO_DATA_FOLDER
```
where `PATH_TO_DATA_FOLDER` is `../ExampleData/OptiTrack/` in this example. The script converts all `.BFL` files it finds in the given folder to a `.csv` with the name `FLIGHT_ID.01.csv` and also generates a useless `FLIGHT_ID.01.events` file. If there already exists a `FLIGHT_ID.01.csv` file, this log is skipped. Thus the script can be run on large folders where only a few new flights are copied into.
Note: The converted file is also provided for reference. If you want to try the conversion, delete the `Ex_OptiTrack.01.csv`

#### Merging Betaflight and OptiTrack Data
The data folder should now contain, for each flight, the following files
- `FlightID.bag` (Rosbag)
- `FlightID.BFL` (Betaflight Log)
- `FlightID_traj.csv` (Trajectory)
- `FlightID.01.csv` (converted blackbox file)
- `FlightID.01.events` (useless file)

If all files are present, the OptiTrack Information can be combined with the Betaflight Logs. For this, use the main script `Matlab/OptiTrack/MergeAndProcessData.m`. For an overview of the options, see the header of the script. With the default options, the following files are produced (column order, see header of the `.csv` files):
- `merged_FlightID.csv` A file containing the merged betaflight and optitrack data over the duration of the whole flight, that is all the time where the betaflight log and the rosbag overlap.
- `merged_FlightID_seg_X.csv` Multiple segment files that contain the interesting part of the full flight, i.e. time segements where the drone moves and is actually airborne. Use those for anything other than visualization of the flights! *Those files are what you're probably looking for*. Their column order (MATLAB indexing) can also be printed with the Matlab command `help mergefile`.
- `raw_merged_FlightID.csv` A file which contains the seperate betaflight and optitrack information in case this is needed for a different script.
- `command_FlightID.csv` All commands sent by the MPC (note, sampling rate of betaflight and not of the OptiTrack to give higher resolution) and what betaflight received.
- `raw_command_FlightID.csv` Only the MPC commands, without any interpolation.
- `Flight_ID.pdf` contains a visualization of the whole flight. This should be inspected manually to check that nothing went wrong. The auto-segmentation is also drawn.

### Application of the Base Model
To apply a base model to the dataset, the simulator needs to be built. Note that this requires the GSL library which can be installed using
```
$ sudo apt-get install libgsl-dev libeigen3-dev
```

Now, build the BEM simulator 
```
$ cd simulator
$ mkdir build
$ cd build
$ cmake ..
$ make
```

Now execute the script passing the path to the datafolder as the first argument. The datafolder is the folder, where the `merged_` files from MATLAB are located, i.e.
```
$ cd Scripts
$ ./applyBM.sh ../ExampleData/OptiTrack/
```

The script can be configured to use a different model other the bem (i.e. fit or none) by changing the `model` variable. This change must also be implemented in the simulator itself by changing the header file `simulator/include/params.h`. The line
```
#define MODEL 1
```
can either be set to `1` (BEM model), `0` (quadratic fit) or `-1` (none model). If the model type was adjusted, the simulator needs to be rebuilt.

The `applyBM` script creates a folder (if it does not exist) with the model name. The folders contain the training data needed in the next step, based on the 'Ex\_OptiTrack' example.

To keep the datasize small enough, the example dataset already contains all the files required in the next step. Therefore, remove the newly created files from the `bem` subfolder and simply delete the `fit` and `none` folder altogether.
```
$ rm ExampleData/OptiTrack/bem/bem_Ex*
$ rm -r ExampleData/OptiTrack/fit
$ rm -r ExampleData/OptiTrack/none
```
The last two commands are only required, if different models have been tried out.

### Train a Neural Network
#### Get the training data
The data folder should now contain a subfolder with the model name, e.g. `bem`. This subfolder needs to contain one (or multiple) `.csv` files with the name `MODELNAME/MODELNAME_FlightID_seg_X.csv`. The training/validation/test split can be achieved using the convenience script found in `Python/data/get_dataset.sh`.

Line 6 and 7 determine how many files are contained in the validation and test dataset. Note that by providing a `testset.txt` file in the same folder as the `get_dataset.sh` script, a hold-out set is created manually. The useage of the script is
```
$ ./get_dataset.sh PATH_TO_ROOT_DATA_FOLDER MODELNAME
```
This copies the files from the `ExampleData` folder to the `MODELNAME/train`, `MODELNAME/validation` and `MODELNAME/test` subfolders in the `Python/data` directory. Perform this operation now to to follow this tutorial, i.e. execute
```
$ cd Python/data
$ ./get_datafiles.bash ../../ExampleData/OptiTrack/ bem
```
and verify that
- `$ ls bem/train/ | wc -l` returns 7
- `$ ls bem/validation/ | wc -l` returns 2
- `$ ls bem/test/ | wc -l` returns 1

#### Network Configuration
The network architecture, data directories, batch size etc. can be configured in `config/bem_settings.yaml`

#### Training the Network
To train the network, execute the `train.py` script
```
$ cd Python
$ python3 train.py --settings_file config/bem_settings.yaml
```
The code saves regular checkpoints to the disk. In case training needs to be resumed from one of the checkpoints, activate the corresponding option in the `config/bem_settings.yaml`. By default, the checkpoints are saved to `Python/train_logs/TIMESTAMP`. To monitor the training progress, start TensorBoard
```
$ cd Python
$ tensorboard --logdir=train_logs
```
Note that the best network so far is saved in the subfolder `best_model` of the

#### Testing the Network
The network can be tested (with an input of all ones). For this, execute the `predict_from_pb.py` script as follows
```
$ cd Python
$ python3 predict_from_pb.py  --log_folder train_logs/TIMESTAMP/
```
The script will print some numbers below the text *This is the output based on a ones input*. Note down those 6 numbers as they will be needed later on. The output could look like this
```
tf.Tensor(
[[[-1.4623649  -1.4394754  -1.0538192   0.96378875 -0.33902097
   -0.1568261 ]]], shape=(1, 1, 6), dtype=float32)
```

#### Running the Network on `.csv` files
To visualize network performance, there is a script that generates network predictions for either a single data file or an entire folder.

```
python3 generate_ablation_study.py --load_folder train_logs/TIMESTAMP --data_root data/bem/test --output_dir .
```

- `load_folder` Folder to load the network from
- `data_root` Path to data (can be a single file or a folder)
- `output_dir` Directory to write plots (as png & csv)

### Deploy the Network with TensorRT
#### Installing TensorRT
Now, a working TensorRT Setup is required, see https://docs.nvidia.com/deeplearning/tensorrt/install-guide/index.html for more information on how to set it up. After setting this up, the user needs to create an environment variable (ideally in the `~/.bashrc`) which tells `cmake` where the root of TensorRT is, e.g.
```
export TRT_ROOT=/home/bfd/Software/TensorRT-7.2.2.3/
```
The folder to which the environment variable points should contain, among others, a `lib`, and `include` and a `python` directory.

#### Conversion to ONNX
To be able to use the trained network with NeuroBEM inside Agilicious, it needs to be converted into a serialized engine which is understood by TensorRT. Such a serialzied engine is specific to each computer, so they can't be transferred.

To generate a serialized engine, the saved model first needs to be converted to the general `onnx` format, which can then be converted to a serialized engine. This is done using `tensorflow-onnx` found here https://github.com/onnx/tensorflow-onnx. Once installed, the saved `.pb` model can be converted to an `onnx` model.

By default, the `.pb` models saved by tensorflow have dynamic batch size. However, for the usage with TensorRT we need to explicitly set the batch size (BS) to 1.

```
$ cd train_logs/TIMESTAMP
$ python3 -m tf2onnx.convert --saved-model best_model/ --output best_model/network.onnx --inputs=input_1:0\[BS,H,FL]
```
where
- `BS` is the batch size (needs to be 1)
- `H` is the length of the history (must match the `settings.yaml` file). In this example `H=20`
- `FL` is the feautre length. In this example `FL=10`

#### ONNX to TensorRT
After the `.pb` model is converted to `.onnx`, it can be serialized. For this, install
https://github.com/onnx/onnx-tensorrt. Assuming the tool is installed under `~/software/onnx-tensorrt` one can use the following command to obtain a serialized `.trt` engine
```
$ cd train_logs/TIMESTAMP
$ ~/software/onnx-tensorrt/build/onnx2trt best_model/network.onnx -o best_model/network.trt
```

As a last step, edit the `all.yaml` found in the `train_logs/TIMESTAMP/` folder. The first line needs to be edited, such that the path points to the `networt.trt` file generated in the step above, e.g.
```
filename: '/home/bfd/code/neuro-bem/Python/train_logs/TIMESTAMP/best_model/network.trt
```

#### Test in TensorRT
Now the serialized engine tester needs to be built. This requires a working TensorRT Setup.
```
$ cd Python/trt/
$ mkdir build && cd build
$ cmake ..
$ make
```
The output of the `cmake` should contain some Lines involving CUDA and TensorRT, e.g.
```
Found CUDA: /usr/local/cuda-11 (found version "11.1")
CUDA Libs: /usr/local/cuda-11/lib64/libcudart_static.a;-lpthread;dl;/usr/lib/x86_64-linux-gnu/librt.so
CUDA Headers: /usr/local/cuda-11/include
TensorRT is available!
NVINFER: /home/bfd/Software/TensorRT-7.2.2.3/lib/libnvinfer.so
NVPARSERS: /home/bfd/Software/TensorRT-7.2.2.3/lib/libnvparsers.so
NVINFERPLUGIN: /home/bfd/Software/TensorRT-7.2.2.3/lib/libnvinfer_plugin.so
CUDNN is available!
CUDNN_LIBRARY: /usr/lib/x86_64-linux-gnu/libcudnn.so
```

After the `make` command, execute the program and pass the path of the serialized engine
```
$ ./test ../../train_logs/TIMESTAMP/best_model/network.trt
```
The numbers printed to the screen must exactly match the ones noted earlier from the `predict_from_pb.py` script. They also need to be in the same order. If this is the case, the serialized engine (and the TensorRT setup) are correct.


______
# Additional Scripts and Tools
### Overview
The additional scripts and tools are not directly required to use NeuroBEM but they can make the life for a user much easier and contain some cool functions which may be useful in the future. This section gives an overview over the available tools.
A more detailed description of the exact functionality is provided at the top of each script. All subroutines are documented. Type `help functionName` to get some information on how to use the functions.

The tools can be broadly divided into three sections:
- trajectory generation
- processing and visualization of experimental data
- closed-loop simulation


### Trajectory Generation
The script can be found under `Matlab/TrajectoryGeneration/GenerateTrajectory.m`. It takes a time-parametrized description of the trajectory as an input and outputs a sampled trajectory to be used with Agilicious.

The usage is straightforward, with the exception of the error-checking in the end. Verify that
- the parameters defined in `subroutines/setupKingfisher.m` are correct
- no motor exceeds a motor command of 1 in the 'Relative Motor Commands' plot
- the trajectory fits inside the arena (see '3D Position' plot)

To write the trajectory to the disk, make sure to set `WRITE=true`.  More information on how to alter the velocity profile and maximum speed is provided at the top of the script.

### Processing and Visualization
#### Rosbag-Only Data
The `MergeAndProcessData.m` script explained above can only be used if *both* Rosbag and Betaflight-Log are available. In case the latter one is not available, the `Matlab/ProcessBag.m` script can be used to get a very similar result. The script takes the `filename` and `basepath` of the bag as an input. It outputs a `.csv` file with the `proc_` prefix which contains the processed and smoothed Rosbag data. Additionally, the smoothing plots can be printed with the SMOOTHPLOTS option

#### Tracking Error
For controller tuning and similar purposes having an estimate of the tracking error available quickly is important. The script `Matlab/OptiTrack/TrackingError.m` provides this functionality.

The input to the script can be one or multiple Rosbags and the trajectory file which was used when the Rosbags were recorded. The script generates a plot over time and prints a combined RMS Error for each trajectory.


### Betaflight Identification
The closed-loop simulation also relies on an accurate model of the betaflight low-level controller. To identify a) the betaflight PID gains and b) the battery modeling parameters two scripts are provided, `Matlab/LowLevelController/BetaflightID_v2.m` and `Matlab/LowLevelController/BatteryModeling.m`. 

Note that the sample dataset provides only one flight, but the scripts can handle multiple experients if the dataset list contains more than one element. All flights are concatenated and then treated as one. Also, the correct thrust map (the one from the SBUS bridge) must be copied into the script folder!

#### Battery Modeling
The battery modeling script completes two tasks: it determines the coefficients of the battery model which can be implemented in agilicious. The battery model outputs a motor-load dependent battery voltage and works only for full batteries. At the moment, this is hardcoded in `include/agilib/simulator/low_level_controller_betaflight.hpp`. 
The script also calculates the coefficients that are needed to map a motor command to a physical motor speed. This linear-sqrt model is also hardcoded in agilicious in the same file. Copy the coefficients directly into the `.hpp` header file.
Lastly, the script outputs two coefficients which are the slope and the offset of the SBUS command. This also needs to be copied into the `.hpp` file.

The plots that are printed to the screen are not directly needed but visualize what is happening.The battery model (plot 1) typically has a lot of noise and hence having many points that are a bit off is no concern. The same is true for the motor rpm model (plot 2) which often has some mismatch at lower rpm, but that is totally ok. The scatter plot (plot 3) shows how the SBUS command generated with the thrust map (x axis) maps to a normalized motor command (y axis). The last plot shows a comparison of the simulated and measured battery voltage over a flight.

#### PID Gain Identification
The `BetaflightID_v2.m` script identifies the PID gains in all three axes and prints them to the command line. They are used in agilicious and need to be hardcoded in `include/agilib/simulator/betaflight/pid_parts.hpp`. The plot that is shown after execution of the script shows the individual motor commands estimated with the betaflight model and their comparison to the measured values. The match will not be perfect, but the motor commands should be very well reconstructed using the identified betaflight model.

The hardcoding of the parameters will be subject to change when NeuroBEM is properly integrated into agilicious. This README will be updated accordingly.


### Helper Functions
Most of the scripts above are written using a set of about 20 helper functions contained in the `Matlab/Common/` and `Matlab/.../subroutines/` folders. The following lists gives an overview over the available functionalities. A more detailed help is available through the MATLAB `help` command.

In `Matlab/Common/`
- `find_consecutive_segments` analyzes where a rosbag contains no missing data and returns time segments
- `find_segments` returns all segments where a variable increases by one, e.g. it finds holes in continuous indices
- `mask_to_segments` takes a binary mask and returns all segements where the mask was 'True'
- `read_bag` reads a rosbag into a matrix
- `read_log` reads a betaflight log csv into a matrix
- `read_rotors_bag` reads a rosbag recorded from RotorS into a matrix
- `read_sim` reads the output csv of agilicious into a matrix
- `read_traj` reads a trajectory csv file into a matrix
- `segments_to_mask` takes a segment array (of indices) and returns a binary mask, that is true inside each segement and false otherwise
- `segment_times_to_segments` takes a segment array (of times) and returns a binary mask, that is true inside each segment and false otherwise
- `write_csv` writes a csv file *with* a header and automatically appends `.csv.` if omiteed.
- `zoh` zero-order hold implementation from https://ch.mathworks.com/matlabcentral/fileexchange/45040-zero-order-hold

In `Matlab/OptiTrack/subroutines/`
- `align_data` precisely aligns (offset and clock skew) two vectors in time. The precision is sub-sampling period and hence the alignment is not index-based but time-vector based.
- `align_to_reference` aligns (offset only, index precision) two datasets.
- `bag_smoother` spline or Savitzky-Golay smoothing for rosbags from the OptiTrack
- `log_smoother` same funcitonality, for betaflight logs
- `correct_quaterion` finds and correcty quaternion flips in a rosbag csv
- `pre_process_bag` corrects quaternions and discards invalid datapoints
- `resample_log` downsamples a betaflight log by taking the average of the measured quantity in each sampling interval
