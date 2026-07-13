function opt=create_options()
%create_options initializes the options struct for the bag and log smoother
% opt=create_options() returns the opt struct required by the log_smoother()
% and bag_smoother() function .

opt = [];

opt.DEBUGPLOTS = false;
opt.ALIGN_ACC = false;
opt.DATAPLOTS = false;
opt.QUAT = false;
opt.SMOOTHPLOTS = false;
opt.SPLINE = true;
opt.INTERPOLATE = true;




end