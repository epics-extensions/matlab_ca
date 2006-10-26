% mca_excas_test
%
% Some examples of using the mca* routines with the 'excas' server

fred = mcaopen('fred')
jane = mcaopen('jane')
janet = mcaopen('janet')

fred_val = mcaget(fred)
j_vals = mcaget([jane janet])

[handles, states] = mcastate()

info = mcainfo(fred)
if info.PVName ~= 'fred'
    warning(['PV fred''s name is ''' info.PVName '''?']);
end

mcamon(fred)
fprintf(1, 'Waiting for monitors...\n', vals);
pause(2)
vals = mcamonevents(fred);
fprintf(1, 'Received %d values for fred\n', vals);

mcacache(fred)
vals = mcamonevents(fred);
fprintf(1, 'Got data, now %d values pending\n', vals);

mcaclose(janet);
mcaclose(jane);
mcaclose(fred);
