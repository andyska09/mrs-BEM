function data_log = read_log(filename)
%data_log: reads a logfile from betaflight
% data = read_log(fname) reads the log file named fname into the matrix
% data. The first column contains the time stamps in s and the other
% columns are returned as follows:
% Log File: 
%	1	loopIteration
%	2	 time (s)
%	22	 vbatLatest (V)
%	23	 amperageLatest (A)
%	25	 gyroADC[0] (rad/s)
%	26	 gyroADC[1] (rad/s)
%	27	 gyroADC[2] (rad/s)
%	28	 accSmooth[0] (m/s/s)
%	29	 accSmooth[1] (m/s/s)
%	30	 accSmooth[2] (m/s/s)
%	31	 debug[0]
%	32	 debug[1]
%	33	 debug[2]
%	34	 debug[3]
%	35	 motor[0]
%	36	 motor[1]
%	37	 motor[2]
%	38	 motor[3]
% The csv should be generated using the command 
% blackbox_decode --unit-rotation rad/s --unit-acceleration m/s2 --unit-height m --debug fname.BFL
% and then post-processed with vim using the commands
%    :2,$g/^\s*\a/d   
%    :2,$s/^\(\(\s*-\?\d\+\.\?\d*,\)\{39}\).*$/\1/g
% where \{39} might need adjustment to the actual number of purely numeric
% columns

data_log = csvread(filename,1);
data_log(:,2) = (data_log(:,2) - data_log(1,2))/1E6;
data_log(:,30) = data_log(:,30);

end