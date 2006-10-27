function sts = mcaput(varargin)
%MCAPUT       - Write values to EPICS Process Variables
%
% MCAPUT(HANDLE1, VALUE1) - one handle, one value
% MCAPUT(HANDLE1,VALUE1, ... , HANDLEN, VALUEN) - handles and values in pairs
%    EPICS STRING values are passed as MATLAB strings. For example:
%    >> mcaput(H,'MATLAB')
%    or cell arrays of strings
%    >> mcaput(H1,'MATLAB',H2,'EPICS')
%
% MCAPUT(HANDLES_CELL_ARRAY,VALUES_CELL_ARRAY) - arguments are grouped
%    in cell array of integer handles and a cell array of values
%    of equal length
% 
% Returns an array of status values: 1-success, 0-failure
% 
% Note (Advanced): MCAPUT is implemented as a call to ca_put_array_callback function
%    in CA client library. MCAPUT returns zero if the server does not confirm the 'put'
%    within the 'put' timeout.
%    !!! HOWEVER the 'put' may still go through after that, and since the 
%    default timeout is very small, it's quite likely to get a negative answer.
%
% Note: The special case of MCAPUT([PV, PV, ...], [SCALAR, SCALAR, ...])
% will simply write the scalar values to the PVs without waiting for the
% callback.
%    
% See also MCAGET, MCATIMEOUT.

if nargin==2
    if iscell(varargin{1}) & iscell(varargin{2})
        % {pv, pv, pv, ...}, {value, value, value, ...}
        if length(varargin{1})~=length(varargin{2})
            error('Cell array of MCA handles and cell array of values must be the same length')
		end
        HANDLES = varargin{1}; VALUES = varargin{2};
        ARGS = reshape([HANDLES(:)';VALUES(:)'],1,2*length(varargin{1}))
        sts = mca(70,ARGS{:});
    elseif isnumeric(varargin{1})
        if length(varargin{1})>1
            if length(varargin{1})~=length(varargin{2})
                error('Array of handles and array of values must be the same length');
            end
	        % [pv, pv, pv, ...], [value, value, value, ...]
            sts = mca(80,varargin{1},varargin{2});
        else
            ARGS = varargin;
			% (pv, value)
            sts = mca(70,ARGS{:});
        end
    else
    	error('Invalid mcaput args, need PV, VALUE');
    end
elseif ~rem(nargin,2)
	% 'pv, value, pv, value, ...'
    sts = mca(70,varargin{:});
else
    error('Incorrect number of inputs, need a sequence of PV, VALUE')
end
