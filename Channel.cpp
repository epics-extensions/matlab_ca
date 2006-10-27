
#include <string.h>
#include "MCAError.h"
#include "Channel.h"
#include "ChannelAccess.h"
#include "db_access.h"

int Channel::NextHandle = 1;

static char *mxStrDup(const char *s)
{
    size_t len = strlen(s)+1;
    char *copy = (char *) mxCalloc(1, len);
    memcpy(copy, s, len);
    mexMakeMemoryPersistent(copy);
    return copy;
}

Channel::Channel(const ChannelAccess *CA, const char *Name)
    : CA(CA),
      Handle(NextHandle++),
      PVName(mxStrDup(Name)),
      ChannelID(0),
      EventID(0),
      MonitorCBString(0),
      HostName(0),
      DataBuffer(0),
      AlarmStatus(0),
      AlarmSeverity(0),
      NumElements(0),
      RequestType(0),
      LastPutStatus(0),
      Cache(0)
{
	ResetEventCount();
    
    int status = ca_create_channel(Name, 0, 0, 0, &ChannelID);
    if (status != ECA_NORMAL)
    {
        MCAError::Error("ca_create_channel: %s\n", ca_message(status));
        ChannelID = 0;
        return;
    }
    status = CA->WaitForSearch();
    if (status != ECA_NORMAL)
    {
        MCAError::Warn("WaitForSearch: %s\n", ca_message(status));
        ca_clear_channel(ChannelID);
        ChannelID = 0;
        Handle = 0;
        return;
    }

    // Obtain the name of the Host where the PV is sourced from.
    HostName = mxStrDup(ca_host_name(ChannelID));

    // Allocate memory for some of the Channel's data structures.
    AllocChanMem();
}

Channel::~Channel()
{
    // Disconnect
    if (ChannelID)
    {
        int status = ca_clear_channel(ChannelID);
        ChannelID = 0;
        if (status != ECA_NORMAL)
            MCAError::Error("ca_clear_channel: %s\n", ca_message(status));
    }

	mxFree(PVName);	
    PVName = 0;

	if (HostName)
    {
		mxFree(HostName);
        HostName = 0;
    }	

	if (DataBuffer)
    {
		mxFree(DataBuffer);
        DataBuffer = 0;
    }

	if (Cache)
    {
		mxDestroyArray(Cache);
        Cache = 0;
    }
}

void Channel::AllocChanMem()
{
	// Allocate space for the data on this channel
	NumElements = ca_element_count(ChannelID);
	RequestType = dbf_type_to_DBR_TIME(ca_field_type(ChannelID));

	// Allocate enough space for the data buffer.
	if (DataBuffer)
		mxFree(DataBuffer);

	DataBuffer = (union db_access_val *) mxCalloc(1, dbr_size_n(RequestType, NumElements));
	mexMakeMemoryPersistent(DataBuffer);

	// Allocate the space for the monitor cache.
	if (Cache)
		mxDestroyArray(Cache);
	if (RequestType == DBR_TIME_STRING)
    {	// Create MATLAB String - originally empty
		if (NumElements == 1)
			Cache = mxCreateString("");
		else {
			Cache = mxCreateCellMatrix(1, NumElements);
			for (int i = 0; i < NumElements; i++) {
				mxArray* mymxArray = mxCreateString("");
				//mexMakeArrayPersistent(mymxArray);
				mxSetCell(Cache, i, mymxArray);
			}
		}
	}
	else
    {	// Create a MATLAB numeric array
		Cache = mxCreateDoubleMatrix(1, NumElements, mxREAL);
	}
	mexMakeArrayPersistent(Cache);
}

int Channel::GetNumElements () const {
	return (NumElements);
}

chtype Channel::GetRequestType () const {
	return (RequestType);
}

bool Channel::IsNumeric() const {

	MCAError Err;

	bool Result;

	switch (RequestType) {
	case DBR_TIME_STRING:
		Result = false;
		break;
	case DBR_TIME_CHAR:
	case DBR_TIME_INT:
	case DBR_TIME_FLOAT:
	case DBR_TIME_ENUM:
	case DBR_TIME_LONG:
	case DBR_TIME_DOUBLE:
		Result = true;
		break;
	default:
		MCAError::Error("Unimplemented Request Type %d in GetRequestTypeStr().",
             RequestType);
	}

	return Result;
}

chid Channel::GetChannelID() const {
	return (ChannelID);
}

const char* Channel::GetRequestTypeStr()  const {

	const char* ReqString;

	switch (RequestType) {
	case DBR_TIME_STRING:
		ReqString = "STRING";
		break;
	case DBR_TIME_INT:
		ReqString = "INT";
		break;
	case DBR_TIME_FLOAT:
		ReqString = "FLOAT";
		break;
	case DBR_TIME_ENUM:
		ReqString = "ENUM";
		break;
	case DBR_TIME_CHAR:
		ReqString = "CHAR";
		break;
	case DBR_TIME_LONG:
		ReqString = "LONG";
		break;
	case DBR_TIME_DOUBLE:
		ReqString = "DOUBLE";
		break;
	default:
		ReqString = "UNKNOWN";
		break;		
	}

	return ReqString;

}

void Channel::GetValueFromCA()
{
	int status;

	status = ca_array_get(RequestType, NumElements, ChannelID, DataBuffer);
	if (status != ECA_NORMAL)
        MCAError::Error("ca_array_get: %s\n", ca_message(status));

	status = CA->WaitForGet();
	if (status != ECA_NORMAL)
        MCAError::Error("GetValueFromCA: %s\n", ca_message(status));

	switch (RequestType)
    {
	case DBR_TIME_INT:
		TimeStamp = ((struct dbr_time_short *) DataBuffer)->stamp;
		AlarmStatus = ((struct dbr_time_short *) DataBuffer)->status;
		AlarmSeverity = ((struct dbr_time_short *) DataBuffer)->severity;
		break;
	case DBR_TIME_FLOAT:
		TimeStamp = ((struct dbr_time_float *) DataBuffer)->stamp;
		AlarmStatus = ((struct dbr_time_float *) DataBuffer)->status;
		AlarmSeverity = ((struct dbr_time_float *) DataBuffer)->severity;
		break;
	case DBR_TIME_ENUM:
		TimeStamp = ((struct dbr_time_enum *) DataBuffer)->stamp;
		AlarmStatus = ((struct dbr_time_enum *) DataBuffer)->status;
		AlarmSeverity = ((struct dbr_time_enum *) DataBuffer)->severity;
		break;
	case DBR_TIME_CHAR:
		TimeStamp = ((struct dbr_time_char *) DataBuffer)->stamp;
		AlarmStatus = ((struct dbr_time_char *) DataBuffer)->status;
		AlarmSeverity = ((struct dbr_time_char *) DataBuffer)->severity;
		break;
	case DBR_TIME_LONG:
		TimeStamp = ((struct dbr_time_long *) DataBuffer)->stamp;
		AlarmStatus = ((struct dbr_time_long *) DataBuffer)->status;
		AlarmSeverity = ((struct dbr_time_long *) DataBuffer)->severity;
		break;
	case DBR_TIME_DOUBLE:
		TimeStamp = ((struct dbr_time_double *) DataBuffer)->stamp;
		AlarmStatus = ((struct dbr_time_double *) DataBuffer)->status;
		AlarmSeverity = ((struct dbr_time_double *) DataBuffer)->severity;
		break;
	case DBR_TIME_STRING:
		TimeStamp = ((struct dbr_time_string *) DataBuffer)->stamp;
		AlarmStatus = ((struct dbr_time_string *) DataBuffer)->status;
		AlarmSeverity = ((struct dbr_time_string *) DataBuffer)->severity;
		break;
	default:
		MCAError::Error("GetValueFromCA(%s): Unimplemented Request Type %d\n",
                        PVName, RequestType);
	}
}

double Channel::GetNumericValue( int Index ) const
{
	double Val;
	MCAError Err;

	switch (RequestType) {
	case DBR_TIME_INT:
		Val = (double) (*(&(DataBuffer->tshrtval.value) + Index));
		break;
	case DBR_TIME_FLOAT:
		Val = (double) (*(&(DataBuffer->tfltval.value) + Index));
		break;
	case DBR_TIME_ENUM:
		Val = (double) (*(&(DataBuffer->tenmval.value) + Index));
		break;
	case DBR_TIME_CHAR:
		Val = (double) (*(&(DataBuffer->tchrval.value) + Index));
		break;
	case DBR_TIME_LONG:
		Val = (double) (*(&(DataBuffer->tlngval.value) + Index));
		break;
	case DBR_TIME_DOUBLE:
		Val = (double) (*(&(DataBuffer->tdblval.value) + Index));
		break;
	case DBR_TIME_STRING:
		MCAError::Error("GetNumericValue(%s) cannot return a string value.",
                        PVName);
	}
	return Val;
}

const char* Channel::GetStringValue ( int Index ) const
{
	char *Str;
	MCAError Err;

	switch (RequestType)
    {
	case DBR_TIME_STRING:
		Str = (char *)(&(DataBuffer->tstrval.value) + Index);
		break;
	default:
		MCAError::Error("GetStringValue(%s) cannot return a string value.",
                        PVName);
	}
	return Str;
}

void Channel::SetNumericValue( int Index, double Value )
{
	MCAError Err;

	chtype Type = dbf_type_to_DBR(ca_field_type(ChannelID));;
	switch (Type) {
	case DBR_INT:
		*((dbr_short_t *) (DataBuffer) + Index) = (dbr_short_t) (Value);
		break;
	case DBR_FLOAT:
		*((dbr_float_t *) (DataBuffer) + Index) = (dbr_float_t) (Value);
		break;
	case DBR_ENUM:
		*((dbr_enum_t *) (DataBuffer) + Index) = (dbr_enum_t) (Value);
		break;
	case DBR_CHAR:
		*((dbr_char_t *) (DataBuffer) + Index) = (dbr_char_t) (Value);
		break;
	case DBR_LONG:
		*((dbr_long_t *) (DataBuffer) + Index) = (dbr_long_t) (Value);
		break;
	case DBR_DOUBLE:
		*((dbr_double_t *) (DataBuffer) + Index) = (dbr_double_t) (Value);
		break;
	case DBR_STRING:
		MCAError::Error("SetNumericValue() cannot take a String value.");
	}

}

void Channel::SetStringValue ( int Index, char* StrBuffer) {

	MCAError Err;

	chtype Type = dbf_type_to_DBR(ca_field_type(ChannelID));;

	switch (Type) {
	case DBR_STRING:
		strcpy((char *)(*((dbr_string_t *)(DataBuffer) + Index)), StrBuffer);
		break;
	default:
		MCAError::Error("SetStringValue() must take a String value.");
	}

}

void Channel::PutValueToCACallback ( int Size, caEventCallBackFunc *PutEventHandler ) {

	int status;
	MCAError Err;

	chtype Type = dbf_type_to_DBR(ca_field_type(ChannelID));;
	status = ca_array_put_callback(Type, Size, ChannelID, DataBuffer, PutEventHandler, this);
	if (status != ECA_NORMAL) {
        MCAError::Error("ca_array_put_callback: %s\n", ca_message(status));
		LastPutStatus = 0;
	}

	status = CA->WaitForPut();

	return;
}

int Channel::PutValueToCA ( int Size ) const {

	int status;
	int RetVal;
	MCAError Err;

	chtype Type = dbf_type_to_DBR(ca_field_type(ChannelID));;
	status = ca_array_put(Type, Size, ChannelID, DataBuffer);
	if (status != ECA_NORMAL)
		RetVal = 0;
	else
		RetVal = 1;

	status = CA->WaitForPutIO();

	return (RetVal);
}

void Channel::SetLastPutStatus( double Status ) {
	LastPutStatus = Status;
}

double Channel::GetLastPutStatus() const {
	return (LastPutStatus);
}

void Channel::SetMonitorString(const char* MonString, int buflen) {

	// If it exists, remove the monitor string.
	//
	if (MonitorCBString) {
		mxFree(MonitorCBString);
		MonitorCBString = NULL;
	}

	// Allocate space for the new monitor string.
	//
	MonitorCBString = (char*) mxMalloc(buflen);
	mexMakeMemoryPersistent(MonitorCBString);
	strcpy(MonitorCBString, MonString);

}

void Channel::LoadMonitorCache( struct event_handler_args arg ) {

	// Pointer to the data associated with the channel.
	//
	union db_access_val *pBuf = (union db_access_val *) arg.dbr;

	int i;
	double *myDblPr;
	mxArray *mymxArray;

	int Cnt = ca_element_count(ChannelID);
	if (Cnt > NumElements)
		AllocChanMem();

	if (arg.type != RequestType)
		AllocChanMem();

	if(RequestType == DBR_TIME_STRING) {
	
		TimeStamp = ((struct dbr_time_string *) pBuf)->stamp;
		if (NumElements == 1) {

			mxDestroyArray(Cache);
			Cache = mxCreateString((char *)((pBuf)->tstrval.value));
			mexMakeArrayPersistent(Cache);

		}
		else {

			for (i = 0; i < NumElements; i++) {

				mymxArray = mxGetCell(Cache, i);
				mxDestroyArray(mymxArray);
				mymxArray = mxCreateString((char *) (&(pBuf->tstrval.value) + i));
				//mexMakeArrayPersistent(mymxArray);
				mxSetCell(Cache, i, mymxArray);

			}
		}
	}
	else {
	
		myDblPr = mxGetPr(Cache);

		switch (RequestType) {
		case DBR_TIME_INT:
			TimeStamp = ((struct dbr_time_short *) pBuf)->stamp;
			AlarmStatus = ((struct dbr_time_short *) pBuf)->status;
			AlarmSeverity = ((struct dbr_time_short *) pBuf)->severity;
			for (i = 0; i < NumElements; i++)
				myDblPr[i] = (double) (*(&((pBuf)->tshrtval.value) + i));
			break;
		case DBR_TIME_FLOAT:
			TimeStamp = ((struct dbr_time_float *) pBuf)->stamp;
			AlarmStatus = ((struct dbr_time_float *) pBuf)->status;
			AlarmSeverity = ((struct dbr_time_float *) pBuf)->severity;
			for (i = 0; i < NumElements; i++)
				myDblPr[i] = (double) (*(&((pBuf)->tfltval.value) + i));
			break;
		case DBR_TIME_ENUM:
			TimeStamp = ((struct dbr_time_enum *) pBuf)->stamp;
			AlarmStatus = ((struct dbr_time_enum *) pBuf)->status;
			AlarmSeverity = ((struct dbr_time_enum *) pBuf)->severity;
			for (i = 0; i < NumElements; i++)
				myDblPr[i] = (double) (*(&((pBuf)->tenmval.value) + i));
			break;
		case DBR_TIME_CHAR:
			TimeStamp = ((struct dbr_time_char *) pBuf)->stamp;
			AlarmStatus = ((struct dbr_time_char *) pBuf)->status;
			AlarmSeverity = ((struct dbr_time_char *) pBuf)->severity;
			for (i = 0; i < NumElements; i++)
				myDblPr[i] = (double) (*(&((pBuf)->tchrval.value) + i));
			break;
		case DBR_TIME_LONG:
			TimeStamp = ((struct dbr_time_long *) pBuf)->stamp;
			AlarmStatus = ((struct dbr_time_long *) pBuf)->status;
			AlarmSeverity = ((struct dbr_time_long *) pBuf)->severity;
			for (i = 0; i < NumElements; i++)
				myDblPr[i] = (double) (*(&((pBuf)->tlngval.value) + i));
			break;
		case DBR_TIME_DOUBLE:
			TimeStamp = ((struct dbr_time_double *) pBuf)->stamp;
			AlarmStatus = ((struct dbr_time_double *) pBuf)->status;
			AlarmSeverity = ((struct dbr_time_double *) pBuf)->severity;
			for (i = 0; i < NumElements; i++)
				myDblPr[i] = (double) (*(&((pBuf)->tdblval.value) + i));
			break;
		default:
			break;
		}
	}
}

const mxArray *Channel::GetMonitorCache() const {
	return (Cache);
}

bool Channel::EventInstalled() const {
	return (EventID==0?0:1);
}

int Channel::AddEvent( caEventCallBackFunc *MonitorEventHandler ) {

	int status;
	MCAError Err;

 	status = ca_add_array_event(RequestType, ca_element_count(ChannelID), ChannelID,
                                  MonitorEventHandler, this, 0.0, 0.0, 0.0, &EventID);
	if (status != ECA_NORMAL)
		MCAError::Error("ca_add_array_event: %s\n", ca_message(status));

	return (status);

}

void Channel::ClearMonitorString() {

	// If it exists, remove the monitor string.
	//
	if (MonitorCBString) {
		mxFree(MonitorCBString);
		MonitorCBString = NULL;
	}

}

void Channel::ClearEvent() {

	int status;
	MCAError Err;

	if (EventID) {

		status = ca_clear_event(EventID);
		if (status != ECA_NORMAL)
			MCAError::Warn("ca_add_array_event failed.\n");

		EventID = 0;

		ClearMonitorString();
	}
}

bool Channel::MonitorStringInstalled() const {
	return (MonitorCBString?1:0);
}

const char* Channel::GetMonitorString() const {
	return (MonitorCBString);
}

