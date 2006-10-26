#ifndef mcaerror_h
#define mcaerror_h

class MCAError
{
public:
    static void Warn(const char *format, ...);
    static void Error(const char *format, ...);
};

#endif // mcaerror_h
