Notes by Hinko Kocevar and Jeffrey Hagen
to compile MCA on Windows 7, 64-bit

Download and install 7zip (http://www.7-zip.org/download.html)
Download and install Make (http://gnuwin32.sourceforge.net/packages/make.htm)
Download and install Perl (http://strawberryperl.com/)
Install Matlab 2013b (if not already installed)

****  Couldn't get this to work  ******
Download and install Microsoft Windows SDK v7.1
 http://se.mathworks.com/matlabcentral/answers/101105-how-do-i-install-microsoft-windows-sdk-7-1?requestedDomain=www.mathworks.com
 https://www.microsoft.com/en-us/download/details.aspx?id=8279 -  Microsoft Windows SDK for Windows 7 and .NET Framework 4

 If it complains about .NET 4 missing, follow the instructions on first link.
 I did not reinstall .NET 4.5 after installing SDK v7.1, that I had to remove in my case.
****************************************
Instead used Visual Studio 2012 Professional
****************************************





Compile EPICS base 3.14.12.5
 Get the sources
  http://www.aps.anl.gov/epics/download/base/baseR3.14.12.5.tar.gz

 Extract to c:\

 Set EPICS_HOST_ARCH in Environment Variables, System Variables to
  EPICS_HOST_ARCH=windows-x64

 Open cmd

****  Altered this step ****************
 Setup 64-bit compiler (see http://stackoverflow.com/questions/20829477/vcvars64-bat-file-is-missing)
 No vcvarsall.bat et. al. needed, just run following command
 
 CALL "c:\Program Files\Microsoft SDKs\Windows\v7.1\Bin\SetEnv.Cmd" /x64
 cmd text turns yellow
***** To *******************************
"C:\Program Files (x86)\Microsoft Visual Studio 11.0\VC\vcvarsall.bat" amd64
****************************************

 Check compiler version that it shows "x64"
  cl \h

***** Instesd of ***********************	
Microsoft (R) C/C++ Optimizing Compiler Version 16.00.30319.01 for x64
Copyright (C) Microsoft Corporation.  All rights reserved.
***** My Compiler Version **************
Microsoft (R) C/C++ Optimizing Compiler Version 17.00.60610.1 for x64
Copyright (C) Microsoft Corporation.  All rights reserved.
****************************************


Adjust LINK_OPT_FLAGS_YES in  base-3.14.12.5\configure\os\CONFIG.win32-x86.win32-x86
 Look for "LINK_OPT_FLAGS_YES" and add "/MANIFEST"
 New content: LINK_OPT_FLAGS_YES = /MANIFEST /LTCG /incremental:no /opt:ref /release $(PROD_VERSION:%=/version:%)

 Compile EPICS base
  cd c:\base-3.14.12.5
  "c:\Program Files (x86)\GnuWin32\bin\make.exe"

Download and compile MCA sources (https://ics-web.sns.ornl.gov/kasemir/mca/index.html)
 At this time R4-4 is latest stable
 Extract to Matlab toolbox folder

Setup MEX C compiler
 mex -setup -v
[...]
Found installed compiler 'Microsoft Windows SDK 7.1 (C)'.
MEX configured to use 'Microsoft Windows SDK 7.1 (C)' for C language compilation.

Setup MEX C++ compiler
 mex -setup -v c++
[...]
Found installed compiler 'Microsoft Windows SDK 7.1 (C++)'.
MEX configured to use 'Microsoft Windows SDK 7.1 (C++)' for C++ language compilation.

Compile MCA 
 Make sure that -DEPICS_NO_DLL is *NOT* specified! Otherwise linking fails due
 to some missing symbols!

 cd c:\MATLAB\R2016a\toolbox\mca-R4-4
 mex mca.cpp MCAError.cpp Channel.cpp c:\base-3.14.12.5\lib\windows-x64\Com.lib c:\base-3.14.12.5\lib\windows-x64\ca.lib -DDB_TEXT_GLBLSOURCE -DMSCC -DWIN32 -DWIN64 -Ic:\base-3.14.12.5\include -Ic:\base-3.14.12.5\include\os\WIN32 -v

 Should have 'mca.mexw64' if all goes well!

****** I didn't do this  ********************
Setup PATH in Environment Variables, System Variables to hold EPICS paths

... ;C:\base-3.14.12.5\bin\windows-x64;C:\base-3.14.12.5\lib\windows-x64

Start matlab

Add paths to MCA

>> addpath c:\MATLAB\R2016a\toolbox\mca-R4-4
>> addpath c:\MATLAB\R2016a\toolbox\mca-R4-4\matlab
******* Instead I did this *******************
Copy mca.mexw64 to .\My Documents\MATLAB\MCA
Copy all files from c:\program files (x86)\R2013b\toolbox\mca-R4-4\matlab to .\My Documents\MATLAB\MCA\matlab
Copy ca.dll and Com.dll from C:\base-3.14.12.5\bin\windows-x64 to .\My Documents\MATLAB\MCA

Start Matlab
Within Matlab right click on MCA directory and select "add folder and subfolders to path"
Easier to enable and disable inclusion this way...
**********************************************

Get PV value

>> pv=mcaopen('LLRF:OPMODE')
MCA Initialized

pv =

     1

>> mcaget(pv)

ans =

     4



DONE!

