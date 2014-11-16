#ifndef HASH3_H
#define HASH3_H

#include "uint256.h"
#include "crypto_hash.h"

using namespace std;

int crypto_hash( unsigned char *out, const unsigned char *in, unsigned long long inlen );

template<typename T1>
inline uint256 Hash3(const T1 pbegin, const T1 pend)
{
    static unsigned char pblank[1];
    uint256 hash1;
    crypto_hash((unsigned char*)&hash1, (pbegin == pend ? pblank : (unsigned char*)&pbegin[0]), (pend - pbegin) * sizeof(pbegin[0]));    
    uint256 hash2;
    crypto_hash((unsigned char*)&hash2, (unsigned char*)&hash1, sizeof(hash1));    
    return hash2;
}

// prototype, need research
template<typename T1>
inline uint256 Hash3_prototype(int blockVersion, const T1 pbegin, const T1 pend)
{
    if (blockVersion == 1)
        return Hash3(pbegin, pend);
    if (blockVersion == 2) // necessary conditions for the ...
        // and add block height dependency
    {
        //Simple, no conditions, for test
        static unsigned char pblank[1];
        uint256 hash1, hash2, hash3;
        crypto_hash((unsigned char*)&hash1, (pbegin == pend ? pblank : (unsigned char*)&pbegin[0]), (pend - pbegin) * sizeof(pbegin[0]));
        SHA256((unsigned char*)&hash1, sizeof(hash1), (unsigned char*)&hash2);
        crypto_hash((unsigned char*)&hash3, (unsigned char*)&hash2, sizeof(hash2));
        return hash3;
    }
    throw runtime_error("Invalid block version");
}


#endif // HASH3_H
