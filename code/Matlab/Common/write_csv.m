function write_csv(fname, data, header)
%write_csv writes a csv file with a header
% write_csv(fname, data, header) writes the data (N by M matrix) to the file
% fname. If a header is provided, it must contain M elements of type string.
% If the filename does not end with ".csv" it is automatically appended.

tmp = convertStringsToChars(fname);
if length(tmp) < 4 || ~all(tmp(end-3:end) == '.csv')
    fname = fname + ".csv";
end

if nargin == 3
    if length(header) ~= size(data,2)
        disp ("Header has wrong number of columns");
        return
    end
    fid = fopen(fname,'w');
    for s=header(1:end-1)
        fprintf(fid, '%s,', s);
    end
    fprintf(fid, '%s\n',header(end));
    fclose(fid);
end

dlmwrite(fname, data, '-append', 'precision', 24);
fprintf("Successfully written %s\n", fname);

end