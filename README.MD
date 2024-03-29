Matlab CA
=========

`matlab_ca` aka `MCA` is an EPICS ChannelAccess client interface for Matlab.

See https://controlssoftware.sns.ornl.gov/mca/

Usage
-----

After successful setup (see below), you should be able to use
the online help:

```
>> help mca/matlab
MCAALARM     - read alarm status and severity for PVs previously read with MCAGET or MCAMON
MCACACHE     - reads locally cached value of a PV. 
MCACHECK     - Same as MCASTATE
MCACHECKOPEN - returns handle(s) to PV(s)
MCACLEARMON  - uninstall monitors, previously installed with MCAMON
MCACLOSE     - permanently closes channels
MCAEXIT      - Shut down channel access...
MCAGET       - read values from PV's
MCAINFO      - get connection status and other information about a PV 
MCAISOPEN    - Check if a channel to a PV is open with MCAOPEN
MCAMON       - install or replace monitor on a PV
MCAMONEVENTS - returns the number of monitor events which have
MCAMONTIMER  - Controls the initialisation and termination of the MATLAB
MCAOPEN      - open a Channel Access connection to an EPICS Process Variable
MCAPOLL      - poll channel access...
MCAPUT       - Write values to EPICS Process Variables
MCASTATE     - returns an array of connection states for open channels
MCATIME      - read timestamps for PVs previously read with MCAGET or MCAMON
MCATIMEOUT   - set or display MCA timeout setings
MCAUNLOCK    - unlocks the MCAMAIN mex-file 
MCAVERSION   - get version info 
```

Start reading at `help mcaopen`, or look at some of the functions
in the `tests/unit_tests.m` file.


Compilation, Setup
------------------

In principle, the Matlab `mex` command takes care of
invoking the correct compiler & linker for your
platform and your version of Matlab.

In addition, however, we need the proper flags
to use the EPICS base includes and libraries.

At the SNS, we use the Makefile,
which supports

```
  make
  make install
  make clean
```

Then, each user has to execute
```
    setup.matlab
```
to include the library and m files for use from within matlab.

System specifics for setups that 'worked' like this:

* Red Hat Enterprise Linux AS release 3, EPICS R3.14.7, Matlab 7.2
  All OK, just define the EPICS_.. and MEX variables
  listed in the start of the Makefile in the environment.

* MacOS X 10.4, EPICS R3.14.8.2, Matlab 7.1 and 7.2.
  To use the default gcc4 which was also used
  to compile EPICS base:
  `cp /Applications/MATLAB72/bin/mexopts.sh .`
  and edit `mexopts.sh`:
  Find the `CC=gcc-3.3` and `CXX=g++-3.3` lines,
  and remove the `-3.3`, so that it's just
  `gcc` resp. `g++`.

Alternate compile scripts
-------------------------

There are several scripts in the alt_compile folder,
which various people have created for their site.
If the Makefile doesn't work for you,
try one of those scripts.

