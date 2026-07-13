close all
clear all
clc


addpath ../Common/
addpath subroutines/

dataset = "Ex_OptiTrack";

% Good Defaults!
opt = create_options();
opt.SMOOTHPLOTS = true;
WRITE = false;

basepath = "../../ExampleData/OptiTrack/";
bagfile = dataset + ".bag";
outfile = "proc_" + dataset;

data_bag = read_bag(basepath + bagfile);
data_bag = pre_process_bag(data_bag);


%% Smooth the two datasets
%             1      2 3 4        5 6 7       8 9 10
% bag_smooth: t acc_[r|p|y] rate_[r|p|y] q_[x|y|z|w] acc_[x|y|z] vel_[x|y|z] pos_[x|y|z]
bag_smooth = bag_smoother(data_bag, opt);

if WRITE
   header = ["t", "ang acc x" "ang acc y", "ang acc z",    ...
            "ang vel x", "ang vel y", "ang vel z",        ...
            "quat x", "quat y", "quat z", "quat w",       ...
            "acc x", "acc y", "acc z",                    ...
            "vel x", "vel y", "vel z",                    ...
            "pos x", "pos y", "pos z"];
    write_csv(basepath + outfile, bag_smooth, header);
end
          
          
          
          
          
          
          
          
          
          
          
          
          
          
          