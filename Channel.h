#ifndef channel_h
#define channel_h

#include <cadef.h>
#include "mex.h"

class ChannelAccess;

class Channel
{
public:
	Channel(const ChannelAccess *CA, const char *Name);

	virtual ~Channel();

	int GetHandle() const
    {   return (Handle); }
    
	chid GetChannelID() const;
    
	char* GetPVName() const
    {   return PVName; }

	char* GetHostName() const
    {
        if (HostName)
            return HostName;
        return "<unknown>";
    }
    
	int GetState () const
    {   return ChannelID ? ca_state(ChannelID) : cs_never_conn; }
    
	int GetNumElements () const;
	chtype GetRequestType() const;
	const char* GetRequestTypeStr() const;
    
    /** @returns True if value is numeric, i.e. anything but string.
     *  @see #GetNumericValue()
     *  @see #GetStringValue()
     */
	bool IsNumeric() const;

    /** Issue a 'get' request to CA.
     *  Must be called first to retrieve the value from CA.
     *  Will create Matlab exception on failure, including timeout.
     */
	void GetValueFromCA();

    /* @return Most recent time stamp. */    
    epicsTimeStamp Channel::GetTimeStamp() const
    {   return TimeStamp; }
    
    /* @return Most recent alarm status. */    
    dbr_short_t Channel::GetAlarmStatus() const
    {   return AlarmStatus; }
    
    /* @return Most recent alarm severity. */    
    dbr_short_t Channel::GetAlarmSeverity() const
    {   return AlarmSeverity; }

    /** Get numeric value.
     *  Use only if isNumeric return true.
     */    
	double GetNumericValue( int ) const;
    
    /** Get string value.
     *  Use only if isNumeric return false.
     */    
	const char* GetStringValue ( int ) const;
	


	// These methods must be used in combination:
	//
	// 1) SetNumericValue - must be called first if IsNumeric() returns True
	// 2) SetStringValue - must be called first if IsNumeric() returns False
	// 3) PutValueToCA - must be called next to send the value to CA
	// 4) SetLastPutStatus - must be called in PutValueToCA callback
	// 5) GetLastPutStatus - must be called last to determine if the Put succeeded
	//
	void SetNumericValue( int, double );
	void SetStringValue ( int, char* );
    
    /** Issue CA put callback, wait for result within timeout.
     *  @return -1.0 on timeout, 0.0 on error, 1.0 if OK.
     */ 
	double PutValueToCACallback (int Size);
    
    /** Simply send current value to CA, no callback.
     *  @return true when OK.
     */
	bool PutValueToCA ( int ) const;

	void SetMonitorString( const char*, int );
	bool MonitorStringInstalled() const;
	const char* GetMonitorString() const;
	void ClearMonitorString();
	void LoadMonitorCache( struct event_handler_args arg );
	int AddEvent( caEventCallBackFunc * );
	bool EventInstalled() const;
	void ClearEvent();

	
    int GetEventCount() const
    {  return EventCount; }

    void IncrementEventCount()
    {  EventCount++; }

    void ResetEventCount()
    {  EventCount = 0; }

    

	const mxArray *GetMonitorCache() const;

private:
    Channel(); // don't use
    
    void AllocChanMem();
    
    static int NextHandle;

    const ChannelAccess* CA;
    int Handle;                 // MCA channel handle.
    char* PVName;               // The name of the Process Variable
	chid ChannelID;				// CA channel identifier.
	evid EventID;				// CA evnet identifier.  If a channel is not
                                // monitored then EventID == NULL.
	char* MonitorCBString;		// Pointer to the MCA callback string for monitor
	char* HostName;				// The name of the host name where the PV connected
    union db_access_val* DataBuffer;
	epicsTimeStamp TimeStamp;	// The time stamp of the most recent data
	dbr_short_t	AlarmStatus;	// The alarm status of the most recent data
	dbr_short_t	AlarmSeverity;	// The alarm severity of the most recent data

	int NumElements;
	chtype RequestType;

    // used for put-callback
    static void put_callback(struct event_handler_args arg);
    epicsEvent put_completed;
    bool last_put_ok;

	mxArray* Cache;
	int EventCount;
};

#endif // channel_h
