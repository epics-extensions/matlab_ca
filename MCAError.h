#ifndef mcaerror_h
#define mcaerror_h

class MCAError  
{
public:
    enum Level { MCAINFO, MCAWARN, MCAERR };
    void Message(Level level, const char *msg);
};

#endif // mcaerror_h
