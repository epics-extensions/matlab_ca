#include <stdio.h>
#include "hash.h"

int main()
{
    static const int N = 5000;
    IntHash<double> hash;
    int i;
    
    puts("Add & find...");
    for (i=0; i<N; ++i)
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
    
    puts("Iterate...");
    IntHashIterator<double> iter(hash);
    i = 0;
    double *d;
    while (d = iter.next())
    {
        ++i;
        if (iter.getKey() != (int)*d  ||
            iter.getValue() != d)
        {
            printf("braindead iter\n");
            return -1;
        }
    }
    if (i != N)
    {
        printf("wrong iter count: %d\n", i);
        return -1;
    }
    
    puts("Find & remove...");
    for (i=0; i<N; ++i)
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