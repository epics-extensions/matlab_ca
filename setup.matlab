
if [ ! -d ~/matlab ]
then
    echo "Creating ~/matlab"
    mkdir ~/matlab
fi
cat >> ~/matlab/startup.m <<END
global is_matlab
eval('is_matlab=length(matlabroot)>0;', 'is_matlab=0;')
addpath $EPICS_EXTENSIONS/src/mca
addpath $EPICS_EXTENSIONS/lib/$EPICS_HOST_ARCH
END

echo "Created ~/matlab/startup.m for using Matlab with EPICS Channel Access"
echo "Welcome To Matlab Channel Access"
