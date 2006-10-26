function unit_tests

% Standard boilerplate for any test .m file
addpath ../matunit
tests = str2func(suite([mfilename '.m']));
[passres, failres, warnres] = runner(tests, 1);

%========================================================================


function testNonsense
try
   mca(9875);
   % Should not get here...
   fail 'Allowed nonsense switch code?'
catch
end

function testCheckDefaultTimeouts
t = mcatimeout;
assertEquals('default search timeout', 1.0, t(1), 0.01); 
assertEquals('default get timeout', 5.0, t(2), 0.01); 
assertEquals('default put timeout', 0.01, t(3), 0.01);

function testSetCustomTimeouts
mcatimeout('open', 5);
mcatimeout('get', 6);
mcatimeout('put', 7);
t = mcatimeout;
assertEquals('custom search timeout', 5.0, t(1), 0.01); 
assertEquals('custom get timeout', 6.0, t(2), 0.01); 
assertEquals('custom put timeout', 7.01, t(3), 0.01);
mcaexit;

function testOneOpenClose
pv = mcaopen('fred');
[h, names] = mcaopen;
assertEquals('One channel', 1, length(h));
assertEquals('Correct name', 'fred', names{1});
mcaclose(pv);
[h, names] = mcaopen;
assertEquals('No more channels', 0, length(h));

function testMutipleOpenClose
[pv, pv2] = mcaopen('fred', 'janet');
[h, names] = mcaopen;
assertEquals('Two channels', 2, length(h));
assertEquals('Correct name', 'fred', names{1});
assertEquals('Correct name', 'janet', names{2});
mcaclose(pv2, pv);
[h, names] = mcaopen;
assertEquals('No more channels', 0, length(h));

function testCellOpenClose
names={'fred', 'janet'};
pvs = mcaopen(names);
[h, names] = mcaopen;
assertEquals('Two channels', 2, length(h));
assertEquals('Correct name', 'fred', names{1});
assertEquals('Correct name', 'janet', names{2});
mcaclose(pvs);
[h, names] = mcaopen;
assertEquals('No more channels', 0, length(h));

function testBasicGet
pv = mcaopen('fred');
val = mcaget(pv);
mcaclose(pv);



