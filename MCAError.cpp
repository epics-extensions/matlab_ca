
#include "mex.h"
#include "MCAError.h"

void MCAError::Message(Level level, const char *msg)
{
    switch (level)
    {
    case MCAINFO:
        mexPrintf(msg);
        mexPrintf("\n");
        break;
    case MCAWARN:
        mexWarnMsgTxt(msg);
        mexPrintf("\n");
        break;
    case MCAERR:
        mexErrMsgTxt(msg);
        break;
    default:
        mexPrintf("MCAError.Message: Invalid Option\n");
    }
}

