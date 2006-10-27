
// System
#ifdef BCC
#include <cstdlib>
#include <iostream>
#include <algorithm>
using namespace std;
#endif

// Matlab
#include "mex.h"
#include "matrix.h"

// Base
#include <shareLib.h>
#include <cadef.h>
#include <epicsMutex.h>
#include <tsDefs.h>
#include <dbDefs.h>

// Local
#include "ChannelAccess.h"
#include "Channel.h"
#include "hash.h"
#include "queue.h"
#include "MCAError.h"

static ChannelAccess *CA = 0;
typedef IntHash<Channel> ChannelHash;
static ChannelHash ChannelTable;

static epicsMutex mutex;
static Queue<int> MonitorQueue;


static int addChannel(const char *name)
{
    if (CA == 0)
    {
        MCAError::Error("addChannel(%s) called without CA", name);
        return 0;
    }
    Channel* Chan = new Channel(CA, name);
    int Handle = Chan->GetHandle();
    if (Handle != 0)
    {
        // Add the channel into the collection of channels
        ChannelTable.insert(Handle, Chan);
    }
    return Handle;
}

void mca_cleanup()
{
	ChannelHash::Iterator iter(ChannelTable);	

	// Delete all the channels from the Channel Table
    while (iter.getValue())
    {
        int key = iter.getKey();
        Channel *Chan = iter.remove();
		int Handle = Chan->GetHandle();
        if (Handle != key)
            MCAError::Warn("mca_cleanup inconsitency: key %d vs. handle %d\n",
                           key, Handle);
		Chan->ClearEvent();
		delete Chan;
	}
    
	// Empty the Monitor Command Queue
	mutex.lock();
	while(!MonitorQueue.IsEmpty())
		int Tmp = MonitorQueue.Dequeue();
	mutex.unlock();
    delete CA;
    CA = 0;
}

void mcaPutEventHandler( struct event_handler_args arg )
{
        Channel *Chan = (Channel *) arg.usr;

        // This callback writes the integer 1 on success
        // and 0 on failure to the channel object specified
        // by void* arg.usr
        //
        if (arg.status == ECA_NORMAL)
                Chan->SetLastPutStatus(1);
        else
                Chan->SetLastPutStatus(0);
}

void mcaMonitorEventHandler( struct event_handler_args arg )
{
	// The channel object passed in by the ca_add_event call.
	Channel *Chan = (Channel *) arg.usr;

	// Load the Channel's cache.
	Chan->LoadMonitorCache(arg);

	// Count the number of outstanding events
	Chan->IncrementEventCount();

	// If a monitor string is installed, store the channel's
	// handle in the queue for subsequent timer-initiated execution.
	//
	if (Chan->MonitorStringInstalled())
    {
		mutex.lock();
		MonitorQueue.Enqueue(Chan->GetHandle());
		mutex.unlock();
	}
}

// Helper for mcainfo, fills one row in array with info for channel
static const char* MCAInfoFields[] = { "Handle", "PVName", "ElementCount", "NativeType", "State", "MCAMessage", "Host" };
static void getChannelInfo(Channel *Chan, mxArray *matrix, int row)
{
    mxSetFieldByNumber(matrix, row, 0, mxCreateScalarDouble(Chan->GetHandle()));
    mxSetFieldByNumber(matrix, row, 1, mxCreateString(Chan->GetPVName()));
    mxSetFieldByNumber(matrix, row, 2, mxCreateScalarDouble(Chan->GetNumElements()));
    switch (Chan->GetState())
    {
    case cs_conn:
        mxSetFieldByNumber(matrix, row, 3, mxCreateString(Chan->GetRequestTypeStr()));
        mxSetFieldByNumber(matrix, row, 4, mxCreateString("connected"));
        mxSetFieldByNumber(matrix, row, 5, mxCreateString("Normal connection"));
        break;

    case cs_closed:
        mxSetFieldByNumber(matrix, row, 3, mxCreateString("unknown"));
        mxSetFieldByNumber(matrix, row, 4, mxCreateString("disconnected"));
        mxSetFieldByNumber(matrix, row, 5, mxCreateString("Permanently disconnected (cleared)"));
        break;

    default:
        mxSetFieldByNumber(matrix, row, 3, mxCreateString("unknown"));
        mxSetFieldByNumber(matrix, row, 4, mxCreateString("disconnected"));
        mxSetFieldByNumber(matrix, row, 5, mxCreateString("Disconnected due to server or network problem"));
        break;
    }
    mxSetFieldByNumber(matrix, row, 6, mxCreateString(Chan->GetHostName()));
}

// All the matlab calls end up in here.
// This routine is ugly, one big switch for all the
// 'mca(...)' calls, and most important there's no
// enforcement of consistent code numbers in the
// mca(..) calls and the switch in here....
void mexFunction(int nlhs, mxArray *plhs[], int nrhs, const mxArray *prhs[])
{
	int status = 0;
	char PVName[PVNAME_SZ + 1];

	// Lazy initialization
	if (CA == 0)
    {
        CA = new ChannelAccess();
		mexAtExit(mca_cleanup);
		mexLock();
	}

	// Obtain the MCA command and then execute it.
    int	commandswitch = (int) mxGetScalar(prhs[0]);
	switch (commandswitch)
    {
    case -1:   // MCAVERSION - This is where the version string gets defined!
        plhs[0] = mxCreateString("4.0");
        break;
        
	case 0:    // MCAUNLOCK - unlocks the mex file so it can be cleared from memory with clear
		mexUnlock();
		break;
	
	case 1:    // MCAOPEN - open channels using strings of PV Names
	{	// Loop through all the supplied PV Names
		for (int i = 0; i < nrhs - 1; i++)
        {
			// First argument of prhs is the command switch
			mxGetString(prhs[i + 1], PVName, PVNAME_SZ+1);
			plhs[i] = mxCreateScalarDouble(addChannel(PVName));
		}
		break;
	}
    
	case 2:    // MCAOPEN - open channels using a cell array of PV Names
	{	// Determine how big the output cell array needs to be and then create it
		int L = mxGetM(prhs[1]) * mxGetN(prhs[1]);
		plhs[0] = mxCreateDoubleMatrix(1, L, mxREAL);
		double *myDblPr = mxGetPr(plhs[0]);

		// Loop through all the supplied PV Names in the cell array
		for (int i = 0; i < L; i++)
        {
			mxArray *mymxArray = mxGetCell(prhs[1], i);
			mxGetString(mymxArray, PVName, PVNAME_SZ+1);
			myDblPr[i] = addChannel(PVName);
		}
		break;
	}
	
	case 3:    // MCAOPEN - Returns two lists:
               //              a) A double matrix of handles of connected PVs
               //              b) a cell array of correcponding PV names.
	{
		int HandlesUsed = ChannelTable.size();
		// Matrix of handles of connected PVs
		plhs[0] = mxCreateDoubleMatrix(1, HandlesUsed, mxREAL);
		double *myDblPr = mxGetPr(plhs[0]);
		// Cell array of PV Names
		plhs[1] = mxCreateCellArray(1, &HandlesUsed);

		// Create an iterator for the Channel Table and get the first element.
        ChannelHash::Iterator iter(ChannelTable);   
		Channel *Chan = iter.getValue();
        int i = 0;
        while (Chan)
        {
            int Handle = Chan->GetHandle();
            if (Handle != iter.getKey())
                MCAError::Warn("mcaclose inconsitency: Handle %d vs. key %d\n",
                                Handle, iter.getKey());
		    myDblPr[i] = Handle;
			mxArray *mymxArray = mxCreateString(Chan->GetPVName());
			mxSetCell(plhs[1], i, mymxArray);
            ++i;
            Chan = iter.next();
		}
		break;
	}

    // 4 used to be MCACHECK, now same as MCASTATE

	case 5:    // MCACLOSE - Close one channel
	{
        for (int i = 0; i < nrhs - 1; i++)
        {
        	int Handle = (int) mxGetScalar(prhs[i + 1]);
    		// Retrieve the Channel from the Channel Table.
    		Channel *Chan = ChannelTable.find(Handle);
    		if (!Chan)
    			MCAError::Error("mcaclose(%d): Invalid handle.", Handle);
    		else 
            {
    			ChannelTable.remove(Handle);
    			delete Chan;
    		}
        }
		break;
	}

	case 10:    // MCAINFO - Returns two lists:
                //              a) A double matrix of handles of PVs
                //              b) Channel information as a MATLAB structure array
	{
		int HandlesUsed = ChannelTable.size();
		if (HandlesUsed > 0)
        {
			plhs[0] = mxCreateDoubleMatrix(1, HandlesUsed, mxREAL);
			double *handles = mxGetPr(plhs[0]);
            plhs[1] = mxCreateStructMatrix(1, HandlesUsed, 7, MCAInfoFields);

			// Create an iterator for the Channel Table and get the first element.
            ChannelHash::Iterator iter(ChannelTable);   
            Channel *Chan = iter.getValue();
            int i = 0;
            while (Chan)
            {
                getChannelInfo(Chan, plhs[1], i);
				handles[i] = Chan->GetHandle();
                ++i;
                Chan = iter.next();
			}
            if (i != HandlesUsed)
                MCAError::Warn("mcainfo: Got %d, expected %d PVs\n",
                               i, HandlesUsed);        
		}
		else
        {
			MCAError::Warn("mcainfo: No connected PVs found.");
			plhs[0] = mxCreateDoubleMatrix(0, 0, mxREAL);
			plhs[1] = mxCreateDoubleMatrix(0, 0, mxREAL);
		}
		break;
	}

	case 11:    // MCAINFO - Returns Channel information for one Channel
	{
		int Handle = (int) mxGetScalar(prhs[1]);
		Channel *Chan = ChannelTable.find(Handle);
		if (!Chan)
            MCAError::Error("mcainfo(%d): Invalid handle.", Handle);
		else
        {
			plhs[0] = mxCreateStructMatrix(1, 1, 7, MCAInfoFields);
            getChannelInfo(Chan, plhs[0], 0);
		}
		break;
	}

    // MCASTATE(no args) - return array (pv, states) of status for all open channels
    //            (1 - OK, 0 - disconnected)
    case 12:
    {
        int HandlesUsed = ChannelTable.size();
        // Matrix of handles of connected PVs
        plhs[0] = mxCreateDoubleMatrix(1, HandlesUsed, mxREAL);
        double *myDblPr0 = mxGetPr(plhs[0]);

        // Matrix of states of connected PVs
        plhs[1] = mxCreateDoubleMatrix(1, HandlesUsed, mxREAL);
        double *myDblPr1 = mxGetPr(plhs[1]);

        // Loop through all the open channels
        if (nrhs == 1)
        {
            if (HandlesUsed > 0)
            {
                ChannelHash::Iterator iter(ChannelTable);   
                Channel *Chan = iter.getValue();
                int i = 0;
                while (Chan)
                {
                    myDblPr0[i] = (double)(Chan->GetHandle());
                    myDblPr1[i] = (double)(Chan->GetState() == 2);
                    ++i;
                    Chan = iter.next();
                }
            }
            else
            {
                mexWarnMsgTxt("No connected PVs found");
                plhs[0] = mxCreateDoubleMatrix(0, 0, mxREAL);
                plhs[1] = mxCreateDoubleMatrix(0, 0, mxREAL);
            }
        }
        break;
    }

    // MCASTATE(pv, pv, ...) - return an array of status for specified channels
    //                         (1 - OK, 0 - disconnected)
    case 13:
    {
        int HandlesUsed = nrhs - 1;

        // Matrix of states of connected PVs
        plhs[0] = mxCreateDoubleMatrix(1, HandlesUsed, mxREAL);
        double *myDblPr = mxGetPr(plhs[0]);
        // Only loop through the supplied channels
        for (int i = 0; i < HandlesUsed; i++)
        {
                int Handle = (int) mxGetScalar(prhs[i + 1]);
                Channel *Chan = ChannelTable.find(Handle);
                if (!Chan)
                    MCAError::Error("mcastate(%d): Invalid handle.", Handle);

                myDblPr[i] = (double)(Chan->GetState() == cs_conn);
        }
        break;
    }

	case 30:    // MCAPOLL - Poll Channel Access
		ca_poll();
		break;

	case 50:    // MCAGET(pv, pv, ...) - Get PV Values by their MCA Handles
	{
		for (int i = 0; i < nrhs - 1; i++)
        {
			int Handle = (int) mxGetScalar(prhs[i + 1]);
			Channel *Chan = ChannelTable.find(Handle);
			if (!Chan)
				MCAError::Error("mcaget(%d): Invalid handle.", Handle);
			// Get the current value from Channel Access.
            // Calls MCAError::Error on failure...
			Chan->GetValueFromCA();
			int Num = Chan->GetNumElements(); 
			if (Chan->GetRequestType() == DBR_TIME_STRING)
            {
				if (Num == 1)
					plhs[i] = mxCreateString(Chan->GetStringValue(0));
				else
                {
					plhs[i] = mxCreateCellMatrix(1, Num);
					for (int j = 0; j < Num; j++)
						mxSetCell(plhs[i], j, mxCreateString(Chan->GetStringValue(j)));
				}
			}
			else
            {
				plhs[i] = mxCreateDoubleMatrix(1, Num, mxREAL);
				double *myDblPr = mxGetPr(plhs[i]);
				for (int j = 0; j < Num; j++)
					myDblPr[j] = Chan->GetNumericValue(j);
			}
		}
		break;
	}

	// MCAGET(pv_array) - Get Scalar PV of the same type
	// Second argument is an array of handles
	// Returns an array of values
	case 51:
	{
		double *myRDblPr = mxGetPr(prhs[1]);
		int M = mxGetM(prhs[1]);
		int N = mxGetN(prhs[1]);
		int NumHandles = M * N;

		plhs[0] = mxCreateDoubleMatrix(M, N, mxREAL);
		double *myLDblPr = mxGetPr(plhs[0]);

		// Issue get requests for each channel
		for (int i = 0; i < NumHandles; i++)
        {
			int Handle = (int) myRDblPr[i];
			Channel *Chan = ChannelTable.find(Handle);
			if (!Chan)
                MCAError::Error("mcaget(%d): Invalid handle.", Handle);

			if (Chan->IsNumeric())
				Chan->GetValueFromCA();
			else
				MCAError::Error(
                "MCAGET(%d) can only be used for numeric PVs when called with an array of PVs",
                Handle);
		}

		// Now retrieve the values for each channel
		//
		for (int j = 0; j < NumHandles; j++) {
			int Handle = (int) myRDblPr[j];
			Channel *Chan = ChannelTable.find(Handle);
			myLDblPr[j] = (double)Chan->GetNumericValue(0);
		}

		break;
	}

	// MCATIME - Returns the EPICS Timestamp for the most recently retrieved value
	//           for a channel (via MCAGET or MCAMON)
	//
	case 60:
	{
		for (int i = 0; i < nrhs - 1; i++) {

			// Retrieve the Channel from the Channel Table.
			// (First argument of prhs is the command switch)
			//
			int Handle = (int) mxGetScalar(prhs[i + 1]);
			Channel *Chan = ChannelTable.find(Handle);
			if (!Chan)
                MCAError::Error("mcatime(%d): Invalid handle.", Handle);

			plhs[i] = mxCreateDoubleMatrix(1, 2, mxREAL);
			double *myDblPr = mxGetPr(plhs[i]);
			myDblPr[0] = (Chan->GetTimeStamp()).secPastEpoch;
			myDblPr[1] = (Chan->GetTimeStamp()).nsec;
		}
		break;
	}

	// MCAALARM - Returns the EPICS Alarm Status and Severity for the most 
	//            recently retrieved value for a channel (via MCAGET or MCAMON)
	//
	case 61:
	{
		for (int i = 0; i < nrhs - 1; i++) {

			// Retrieve the Channel from the Channel Table.
			// (First argument of prhs is the command switch)
			//
			int Handle = (int) mxGetScalar(prhs[i + 1]);
			Channel *Chan = ChannelTable.find(Handle);
			if (!Chan)
                MCAError::Error("mcaalarm(%d): Invalid handle.", Handle);

			plhs[i] = mxCreateDoubleMatrix(1, 2, mxREAL);
			double *myDblPr = mxGetPr(plhs[i]);
			myDblPr[0] = (double)Chan->GetAlarmStatus();
			myDblPr[1] = (double)Chan->GetAlarmSeverity();
		}
		break;
	}

	// MCAPUT - Put values to PVs by their MCA Handles
	//
	case 70:
	{

		int i;
		double *myDblPr;

		dbr_string_t StrBuffer;

		// Following the first argument, which is the command switch, the
		// values for each channel come in pairs
		//
		int NumHandles = (nrhs - 1) /2;

		// Process each channel...
		//
		for (i = 0; i < NumHandles; i++)
                {
			int j = 2 + i * 2;

			// Get the handle and find the channel
			//
			int Handle = (int) mxGetScalar(prhs[1 + i * 2]);

			Channel *Chan = ChannelTable.find(Handle);
			if (!Chan)
                MCAError::Error("mcaput(%d): Invalid handle.", Handle);

			chtype RequestType = Chan->GetRequestType();
			int Num = Chan->GetNumElements(); 

			// Get the value to write
			//
			int L;
			if (mxIsChar(prhs[j]))
				L = 1;
			else
                        {       // min(mxGetNumberOfElements(prhs[j]), Num)
				L = mxGetNumberOfElements(prhs[j]);
				if (Num < L)
                                    L = Num;
                        }

			if (RequestType == DBR_TIME_STRING) {

				if (mxIsChar(prhs[j])) {

					mxGetString(prhs[j], StrBuffer, sizeof(dbr_string_t));
					Chan->SetStringValue(0, StrBuffer);

				}
				else if (mxIsCell(prhs[j])) {

					for (int k = 0; k < L; k++) {
						
						mxGetString(mxGetCell(prhs[j], k), StrBuffer, sizeof(dbr_string_t));
						Chan->SetStringValue(k, StrBuffer);
					}
				}
			}
			else {
				
				// Set the value into the Buffer
				//
				myDblPr = mxGetPr(prhs[j]);
				for (int k = 0; k < L; k++)
					Chan->SetNumericValue(k, myDblPr[k]);

			}

			// Put the values to Channel Access
			//
			// This used to be: Chan->PutValueToCA(L);
                        Chan->PutValueToCACallback(L, mcaPutEventHandler);
		}

		plhs[0] = mxCreateDoubleMatrix(1, NumHandles, mxREAL);
		myDblPr = mxGetPr(plhs[0]);

		for (i = 0; i < NumHandles; i++)
                {
                        int Handle = (int) mxGetScalar(prhs[1 + i * 2]);
                        Channel *Chan = ChannelTable.find(Handle);
                        myDblPr[i] = Chan->GetLastPutStatus();

			// Before use of PutValueToCACallback:
                        // myDblPr[i] = 1;
		}

		break;
	}

	// MCAPUT - Put values to scalar numeric PVs by their MCA Handles
	//
	case 80:
	{
		double *myDblPr;
		int M = mxGetM(prhs[1]);
		int N = mxGetN(prhs[1]);
		int NumHandles = M * N;

		plhs[0] = mxCreateDoubleMatrix(M, N, mxREAL);

		for (int i = 0; i < NumHandles; i++) {

			int Handle = (int) (*(mxGetPr(prhs[1]) + i));

			// Find the channel with the specified handle
			//
			Channel *Chan = ChannelTable.find(Handle);
			if (!Chan)
                MCAError::Error("mcaput(%d): Invalid handle.", Handle);

			if (Chan->IsNumeric()) {

				// Write the value to the PV
				//
				double Value = (*(mxGetPr(prhs[2]) + i));
				Chan->SetNumericValue(0, Value);					
				int status = Chan->PutValueToCA(1);
				myDblPr = mxGetPr(plhs[0]);
                                myDblPr[i] = status;
			}
			else
				MCAError::Error("MCAPUT(80) can only be used for numeric PVs.");

		}
		break;
	}

	// MCAMON - install a monitor
	//
	case 100:
	{

		int Handle = (int) mxGetScalar(prhs[1]);
	
		// Find the channel with the specified handle
		//
		Channel *Chan = ChannelTable.find(Handle);
		if (!Chan)
             MCAError::Error("mcamon(%d): Invalid handle.", Handle);

		// If a monitor event is installed, then just replace the monitor string.
		//
		if (Chan->EventInstalled()) {

			// Clear old monitor callback string
			//
			Chan->ClearMonitorString();

			// If a third argument is specified
			//
			if (nrhs > 2) {
				
				// If the third argument is a string
				//
				if (mxIsChar(prhs[2])) {

					// Create the new monitor callback string
					//
					int buflen = mxGetM(prhs[2]) * mxGetN(prhs[2]) + 1;

					char *CBString = (char *)mxMalloc(buflen);
					mxGetString(prhs[2], CBString, buflen);
					Chan->SetMonitorString(CBString, buflen);
					mxFree(CBString);

				}
				else
					MCAError::Error("Third argument to mcamon must be a string.");
			}
			plhs[0] = mxCreateScalarDouble(1);

		}

		// There is no monitor installed.  Create the monitor string
		// and then register the event.
		//
		else {

			// If a third argument is specified
			//
			if (nrhs > 2) {
				
				// If the third argument is a string
				//
				if (mxIsChar(prhs[2])) {

					// Create the new monitor callback string
					//
					int buflen = mxGetM(prhs[2]) * mxGetN(prhs[2]) + 1;

					char *CBString = (char *)mxMalloc(buflen);
					mxGetString(prhs[2], CBString, buflen);
					Chan->SetMonitorString(CBString, buflen);
					mxFree(CBString);

				}
				else
					MCAError::Error("Third argument to mcamon must be a string.");
			}
			else

				// Make sure the monitor string is cleared.
				//
				Chan->ClearMonitorString();

			int status = Chan->AddEvent(mcaMonitorEventHandler);
			if (status == ECA_NORMAL) {
				ca_poll();
				plhs[0] = mxCreateScalarDouble(1);
			}
			else
				plhs[0] = mxCreateScalarDouble(0);
		}
		break;
	}

	// MCACLEARMON - clears a monitor
	//
	case 200:
	{
		int Handle = (int) mxGetScalar(prhs[1]);
	
		// Find the channel with the specified handle
		//
		Channel *Chan = ChannelTable.find(Handle);
		if (!Chan)
            MCAError::Error("mcaclearmon(%d): Invalid handle.", Handle);

		Chan->ClearEvent();
		
		break;
	}

	// MCACACHE - get the cached values of a monitored PV
	//
	case 300:
	{
		int Handle;
		const mxArray *Cache;

		for (int i = 0; i < nrhs - 1; i++) {

			Handle = (int) mxGetScalar(prhs[i + 1]);
			
			// Find the channel with the specified handle
			//
			Channel *Chan = ChannelTable.find(Handle);
			if (!Chan)
                MCAError::Error("mcacache(%d): Invalid handle.", Handle);

			Cache = Chan->GetMonitorCache();
			if (Cache)
				plhs[i] = mxDuplicateArray(Cache);
			else
				plhs[i] = mxCreateDoubleMatrix(0, 0, mxREAL);
			Chan->ResetEventCount();

		}
		break;
	}

	// MCAMON - get info on installed monitors.
	//
	case 500:
	{
		int i, MonitorsInstalled = 0;
		int HandlesUsed;
		int *HandleArray;
		double *myDblPr;

		// Get the total number of defined channels and create an array
		// to hold the handles of those that have installed monitors.
		//
		HandlesUsed = ChannelTable.size();
		HandleArray = (int *) mxCalloc(HandlesUsed, sizeof(int));

		// Iterate through all the defined channels and add their
		// handles into the array if they have monitors defined.
        ChannelHash::Iterator iter(ChannelTable);   
        Channel *Chan = iter.getValue();
        while (Chan)
        {
			if (Chan->EventInstalled())
				HandleArray[MonitorsInstalled++] = Chan->GetHandle();
            Chan = iter.next();
		}

		if (MonitorsInstalled > 0)
        {
			// An array of the handles of channels with installed monitors.
			plhs[0] = mxCreateDoubleMatrix(1, MonitorsInstalled, mxREAL);
			myDblPr = mxGetPr(plhs[0]);

			// An array of the monitor strings for each installed monitor.
			plhs[1] = mxCreateCellMatrix(1, MonitorsInstalled);

			// Fill up the arrays.
			for (i = 0; i < MonitorsInstalled; i++)
            {
				Chan = ChannelTable.find(HandleArray[i]);
				myDblPr[i] = (double) HandleArray[i];
				mxSetCell(plhs[1], i, mxCreateString(Chan->GetMonitorString()));
			}
		}
		else {
			plhs[0] = mxCreateDoubleMatrix(0, 0, mxREAL);
			plhs[1] = mxCreateCellMatrix(0, 0);
		
		}
		mxFree (HandleArray);

		break;
	}

	// MCAMONEVENTS - Event count for monitors
	//
	case 510:
	{
		double *myDblPr0;
		double *myDblPr1;
		int HandlesUsed = ChannelTable.size();

		plhs[0] = mxCreateDoubleMatrix(1, HandlesUsed, mxREAL);
		myDblPr0 = mxGetPr(plhs[0]);
		plhs[1] = mxCreateDoubleMatrix(1, HandlesUsed, mxREAL);
		myDblPr1 = mxGetPr(plhs[1]);

		ChannelHash::Iterator iter(ChannelTable);	
		Channel *Chan = iter.getValue();
        int i = 0;
        while (Chan)
        {
			myDblPr0[i] = Chan->GetHandle();
			myDblPr1[i] = Chan->GetEventCount();
            ++i;
            Chan = iter.next();
		}
		break;
	}

	// MCAEXEC - Execute the command strings for the channels in
	//			 the monitor queue
	//
	case 600:
	{
		mutex.lock();
		while(!MonitorQueue.IsEmpty())
        {
			int Handle = MonitorQueue.Dequeue();
			mutex.unlock();
			// Find the channel.  If it still exists, execute its 
			// command string (if it has one.)
			Channel *Chan = ChannelTable.find(Handle);
			if (Chan)
            {
				if (Chan->MonitorStringInstalled())
                {
					const char* MonitorString = Chan->GetMonitorString();
					mexEvalString(MonitorString);
				}
			}
			mutex.lock();
		}
		mutex.unlock();
		break;
	}

	case 999:     // MCAEXIT
		mca_cleanup();
		break;

	case 1001:    // Set MCA_SEARCH_TIMEOUT
		CA->SetSearchTimeout(mxGetScalar(prhs[1]));
		break;

	case 1002:    // Set MCA_GET_TIMEOUT
		CA->SetGetTimeout(mxGetScalar(prhs[1]));
		break;

	case 1003:    // Set MCA_PUT_TIMEOUT
		CA->SetPutTimeout(mxGetScalar(prhs[1]));
		break;

    case 1004:	// Set default timeouts
		CA->SetDefaultTimeouts();
        // FALL THROUGH to return the settings
    case 1000:    // Get all Timeout Settings.
    {
        plhs[0] = mxCreateDoubleMatrix(3, 1, mxREAL);
        double *myDblPr = mxGetPr(plhs[0]);
        myDblPr[0] = CA->GetSearchTimeout();
        myDblPr[1] = CA->GetGetTimeout();
        myDblPr[2] = CA->GetPutTimeout();
        break;
    }

	default:
		MCAError::Error("Invalid mca code %d", commandswitch);
	}  // switch (command)
}
