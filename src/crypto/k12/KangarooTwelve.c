/*
Implementation by Ronny Van Keer, hereby denoted as "the implementer".

For more information, feedback or questions, please refer to our website:
https://keccak.team/

To the extent possible under law, the implementer has waived all copyright
and related or neighboring rights to the source code in this file.
http://creativecommons.org/publicdomain/zero/1.0/
*/

#include <string.h>
#include "KangarooTwelve.h"
#ifndef KeccakP1600timesN_excluded
    // #include "KeccakP-1600-times2-SnP.h"
    // #include "KeccakP-1600-times4-SnP.h"
    // #include "KeccakP-1600-times8-SnP.h"
#endif

#define chunkSize       8192
#define laneSize        8
#define suffixLeaf      0x0B /* '110': message hop, simple padding, inner node */

#define security        128
#define capacity        (2*security)
#define capacityInBytes (capacity/8)
#define capacityInLanes (capacityInBytes/laneSize)
#define rate            (1600-capacity)
#define rateInBytes     (rate/8)
#define rateInLanes     (rateInBytes/laneSize)

#define ParallelSpongeFastLoop( Parallellism ) \
    while ( inLen >= Parallellism * chunkSize ) { \
        ALIGN(KeccakP1600times##Parallellism##_statesAlignment) unsigned char states[KeccakP1600times##Parallellism##_statesSizeInBytes]; \
        unsigned char intermediate[Parallellism*capacityInBytes]; \
        unsigned int localBlockLen = chunkSize; \
        const unsigned char * localInput = input; \
        unsigned int i; \
        unsigned int fastLoopOffset; \
        \
        KeccakP1600times##Parallellism##_StaticInitialize(); \
        KeccakP1600times##Parallellism##_InitializeAll(states); \
        fastLoopOffset = KeccakP1600times##Parallellism##_12rounds_FastLoop_Absorb(states, rateInLanes, chunkSize / laneSize, rateInLanes, localInput, Parallellism * chunkSize); \
        localBlockLen -= fastLoopOffset; \
        localInput += fastLoopOffset; \
        for ( i = 0; i < Parallellism; ++i, localInput += chunkSize ) { \
            KeccakP1600times##Parallellism##_AddBytes(states, i, localInput, 0, localBlockLen); \
            KeccakP1600times##Parallellism##_AddByte(states, i, suffixLeaf, localBlockLen); \
            KeccakP1600times##Parallellism##_AddByte(states, i, 0x80, rateInBytes-1); \
        } \
        KeccakP1600times##Parallellism##_PermuteAll_12rounds(states); \
        input += Parallellism * chunkSize; \
        inLen -= Parallellism * chunkSize; \
        ktInstance->blockNumber += Parallellism; \
        KeccakP1600times##Parallellism##_ExtractLanesAll(states, intermediate, capacityInLanes, capacityInLanes ); \
        if (KeccakWidth1600_12rounds_SpongeAbsorb(&ktInstance->finalNode, intermediate, Parallellism * capacityInBytes) != 0) return 1; \
    }

#define ParallelSpongeLoop( Parallellism ) \
    while ( inLen >= Parallellism * chunkSize ) { \
        ALIGN(KeccakP1600times##Parallellism##_statesAlignment) unsigned char states[KeccakP1600times##Parallellism##_statesSizeInBytes]; \
        unsigned char intermediate[Parallellism*capacityInBytes]; \
        unsigned int localBlockLen = chunkSize; \
        const unsigned char * localInput = input; \
        unsigned int i; \
        \
        KeccakP1600times##Parallellism##_StaticInitialize(); \
        KeccakP1600times##Parallellism##_InitializeAll(states); \
        while(localBlockLen >= rateInBytes) { \
            KeccakP1600times##Parallellism##_AddLanesAll(states, localInput, rateInLanes, chunkSize / laneSize); \
            KeccakP1600times##Parallellism##_PermuteAll_12rounds(states); \
            localBlockLen -= rateInBytes; \
            localInput += rateInBytes; \
           } \
        for ( i = 0; i < Parallellism; ++i, localInput += chunkSize ) { \
            KeccakP1600times##Parallellism##_AddBytes(states, i, localInput, 0, localBlockLen); \
            KeccakP1600times##Parallellism##_AddByte(states, i, suffixLeaf, localBlockLen); \
            KeccakP1600times##Parallellism##_AddByte(states, i, 0x80, rateInBytes-1); \
        } \
        KeccakP1600times##Parallellism##_PermuteAll_12rounds(states); \
        input += Parallellism * chunkSize; \
        inLen -= Parallellism * chunkSize; \
        ktInstance->blockNumber += Parallellism; \
        KeccakP1600times##Parallellism##_ExtractLanesAll(states, intermediate, capacityInLanes, capacityInLanes ); \
        if (KeccakWidth1600_12rounds_SpongeAbsorb(&ktInstance->finalNode, intermediate, Parallellism * capacityInBytes) != 0) return 1; \
    }

static unsigned int right_encode( unsigned char * encbuf, size_t value )
{
    unsigned int n, i;
    size_t v;

    for ( v = value, n = 0; v && (n < sizeof(size_t)); ++n, v >>= 8 )
        ; /* empty */
    for ( i = 1; i <= n; ++i )
        encbuf[i-1] = (unsigned char)(value >> (8 * (n-i)));
    encbuf[n] = (unsigned char)n;
    return n + 1;
}

int KangarooTwelve_Initialize(KangarooTwelve_Instance *ktInstance, size_t outputLen)
{
    ktInstance->fixedOutputLength = outputLen;
    ktInstance->queueAbsorbedLen = 0;
    ktInstance->blockNumber = 0;
    ktInstance->phase = ABSORBING;
    return KeccakWidth1600_12rounds_SpongeInitialize(&ktInstance->finalNode, rate, capacity);
}

int KangarooTwelve_Update(KangarooTwelve_Instance *ktInstance, const unsigned char *input, size_t inLen)
{
    if (ktInstance->phase != ABSORBING)
        return 1;

    if ( ktInstance->blockNumber == 0 ) {
        /* First block, absorb in final node */
        unsigned int len = (inLen < (chunkSize - ktInstance->queueAbsorbedLen)) ? inLen : (chunkSize - ktInstance->queueAbsorbedLen);
        if (KeccakWidth1600_12rounds_SpongeAbsorb(&ktInstance->finalNode, input, len) != 0)
            return 1;
        input += len;
        inLen -= len;
        ktInstance->queueAbsorbedLen += len;
        if ( (ktInstance->queueAbsorbedLen == chunkSize) && (inLen != 0) ) {
            /* First block complete and more input data available, finalize it */
            const unsigned char padding = 0x03; /* '110^6': message hop, simple padding */
            ktInstance->queueAbsorbedLen = 0;
            ktInstance->blockNumber = 1;
            if (KeccakWidth1600_12rounds_SpongeAbsorb(&ktInstance->finalNode, &padding, 1) != 0)
                return 1;
            ktInstance->finalNode.byteIOIndex = (ktInstance->finalNode.byteIOIndex + 7) & ~7; /* Zero padding up to 64 bits */
        }
    }
    else if ( ktInstance->queueAbsorbedLen != 0 ) {
        /* There is data in the queue, absorb further in queue until block complete */
        unsigned int len = (inLen < (chunkSize - ktInstance->queueAbsorbedLen)) ? inLen : (chunkSize - ktInstance->queueAbsorbedLen);
        if (KeccakWidth1600_12rounds_SpongeAbsorb(&ktInstance->queueNode, input, len) != 0)
            return 1;
        input += len;
        inLen -= len;
        ktInstance->queueAbsorbedLen += len;
        if ( ktInstance->queueAbsorbedLen == chunkSize ) {
            unsigned char intermediate[capacityInBytes];
            ktInstance->queueAbsorbedLen = 0;
            ++ktInstance->blockNumber;
            if (KeccakWidth1600_12rounds_SpongeAbsorbLastFewBits(&ktInstance->queueNode, suffixLeaf) != 0)
                return 1;
            if (KeccakWidth1600_12rounds_SpongeSqueeze(&ktInstance->queueNode, intermediate, capacityInBytes) != 0)
                return 1;
            if (KeccakWidth1600_12rounds_SpongeAbsorb(&ktInstance->finalNode, intermediate, capacityInBytes) != 0)
                return 1;
        }
    }

    #if defined(KeccakP1600times8_implementation) && !defined(KeccakP1600times8_isFallback)
    #if defined(KeccakP1600times8_12rounds_FastLoop_supported)
    ParallelSpongeFastLoop( 8 )
    #else
    ParallelSpongeLoop( 8 )
    #endif
    #endif

    #if defined(KeccakP1600times4_implementation) && !defined(KeccakP1600times4_isFallback)
    #if defined(KeccakP1600times4_12rounds_FastLoop_supported)
    ParallelSpongeFastLoop( 4 )
    #else
    ParallelSpongeLoop( 4 )
    #endif
    #endif

    #if defined(KeccakP1600times2_implementation) && !defined(KeccakP1600times2_isFallback)
    #if defined(KeccakP1600times2_12rounds_FastLoop_supported)
    ParallelSpongeFastLoop( 2 )
    #else
    ParallelSpongeLoop( 2 )
    #endif
    #endif

    while ( inLen > 0 ) {
        unsigned int len = (inLen < chunkSize) ? inLen : chunkSize;
        if (KeccakWidth1600_12rounds_SpongeInitialize(&ktInstance->queueNode, rate, capacity) != 0)
            return 1;
        if (KeccakWidth1600_12rounds_SpongeAbsorb(&ktInstance->queueNode, input, len) != 0)
            return 1;
        input += len;
        inLen -= len;
        if ( len == chunkSize ) {
            unsigned char intermediate[capacityInBytes];
            ++ktInstance->blockNumber;
            if (KeccakWidth1600_12rounds_SpongeAbsorbLastFewBits(&ktInstance->queueNode, suffixLeaf) != 0)
                return 1;
            if (KeccakWidth1600_12rounds_SpongeSqueeze(&ktInstance->queueNode, intermediate, capacityInBytes) != 0)
                return 1;
            if (KeccakWidth1600_12rounds_SpongeAbsorb(&ktInstance->finalNode, intermediate, capacityInBytes) != 0)
                return 1;
        }
        else
            ktInstance->queueAbsorbedLen = len;
    }

    return 0;
}

int KangarooTwelve_Final(KangarooTwelve_Instance *ktInstance, unsigned char * output, const unsigned char * customization, size_t customLen)
{
    unsigned char encbuf[sizeof(size_t)+1+2];
    unsigned char padding;

    if (ktInstance->phase != ABSORBING)
        return 1;

    /* Absorb customization | right_encode(customLen) */
    if ((customLen != 0) && (KangarooTwelve_Update(ktInstance, customization, customLen) != 0))
        return 1;
    if (KangarooTwelve_Update(ktInstance, encbuf, right_encode(encbuf, customLen)) != 0)
        return 1;

    if ( ktInstance->blockNumber == 0 ) {
        /* Non complete first block in final node, pad it */
        padding = 0x07; /*  '11': message hop, final node */
    }
    else {
        unsigned int n;

        if ( ktInstance->queueAbsorbedLen != 0 ) {
            /* There is data in the queue node */
            unsigned char intermediate[capacityInBytes];
            ++ktInstance->blockNumber;
            if (KeccakWidth1600_12rounds_SpongeAbsorbLastFewBits(&ktInstance->queueNode, suffixLeaf) != 0)
                return 1;
            if (KeccakWidth1600_12rounds_SpongeSqueeze(&ktInstance->queueNode, intermediate, capacityInBytes) != 0)
                return 1;
            if (KeccakWidth1600_12rounds_SpongeAbsorb(&ktInstance->finalNode, intermediate, capacityInBytes) != 0)
                return 1;
        }
        --ktInstance->blockNumber; /* Absorb right_encode(number of Chaining Values) || 0xFF || 0xFF */
        n = right_encode(encbuf, ktInstance->blockNumber);
        encbuf[n++] = 0xFF;
        encbuf[n++] = 0xFF;
        if (KeccakWidth1600_12rounds_SpongeAbsorb(&ktInstance->finalNode, encbuf, n) != 0)
            return 1;
        padding = 0x06; /* '01': chaining hop, final node */
    }
    if (KeccakWidth1600_12rounds_SpongeAbsorbLastFewBits(&ktInstance->finalNode, padding) != 0)
        return 1;
    if ( ktInstance->fixedOutputLength != 0 ) {
        ktInstance->phase = FINAL;
        return KeccakWidth1600_12rounds_SpongeSqueeze(&ktInstance->finalNode, output, ktInstance->fixedOutputLength);
    }
    ktInstance->phase = SQUEEZING;
    return 0;
}

int KangarooTwelve_Squeeze(KangarooTwelve_Instance *ktInstance, unsigned char * output, size_t outputLen)
{
    if (ktInstance->phase != SQUEEZING)
        return 1;
    return KeccakWidth1600_12rounds_SpongeSqueeze(&ktInstance->finalNode, output, outputLen);
}

int KangarooTwelve( const unsigned char * input, size_t inLen, unsigned char * output, size_t outLen, const unsigned char * customization, size_t customLen )
{
    KangarooTwelve_Instance ktInstance;

    if (outLen == 0)
        return 1;
    if (KangarooTwelve_Initialize(&ktInstance, outLen) != 0)
        return 1;
    if (KangarooTwelve_Update(&ktInstance, input, inLen) != 0)
        return 1;
    return KangarooTwelve_Final(&ktInstance, output, customization, customLen);
}
