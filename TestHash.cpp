#include <stdio.h>
#include "hash.h"

int main()
{
    static const int N = 50000;
    IntHash<double> hash;
    
    puts("Add & find...");
    for (int i=0; i<N; ++i)
    {
        double *d = new double;
        *d = (double)i;
        hash.insert(i, d);
        double *f = hash.find(i);
        if (!f)
        {
            printf("not found: %d\n", i);
            return -1;
        }
        if (*f != *d)
        {
            printf("wrong item: %g != %g\n", *d, *f);
            return -1;
        }
    }
    if (hash.size() != N)
    {
        printf("wrong size: %d\n", hash.size());
        return -1;
    }
    
    puts("Find & remove...");
    for (int i=0; i<N; ++i)
    {
        double *f = hash.find(i);
        if (!f)
        {
            printf("not found: %d\n", i);
            return -1;
        }
        if (i != (int)*f)
        {
            printf("wrong item: %d != %g\n", i, *f);
            return -1;
        }
        hash.remove(i);
        delete f;
    }
    if (hash.size() != 0)
    {
        printf("wrong size: %d\n", hash.size());
        return -1;
    }
    puts("OK.");
        
    return 0;
}