#include "mtwister.h"

int main()
{
    MTRand e = seedRand(0);
    long v = genRandLong(&e);
}