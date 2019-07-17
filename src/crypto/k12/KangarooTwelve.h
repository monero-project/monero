/*
Implementation by Ronny Van Keer, hereby denoted as "the implementer".

For more information, feedback or questions, please refer to our website:
https://keccak.team/

To the extent possible under law, the implementer has waived all copyright
and related or neighboring rights to the source code in this file.
http://creativecommons.org/publicdomain/zero/1.0/
*/

#ifndef _KangarooTwelve_h_
#define _KangarooTwelve_h_

#ifndef KeccakP1600_excluded

#include <stddef.h>
#include "align.h"
#include "KeccakSpongeWidth1600.h"
#include "Phases.h"

typedef KCP_Phases KangarooTwelve_Phases;

typedef struct {
    KeccakWidth1600_12rounds_SpongeInstance queueNode;
    KeccakWidth1600_12rounds_SpongeInstance finalNode;
    size_t fixedOutputLength;
    size_t blockNumber;
    unsigned int queueAbsorbedLen;
    KangarooTwelve_Phases phase;
} KangarooTwelve_Instance;

/** Extendable ouput function KangarooTwelve.
  * @param  input           Pointer to the input message (M).
  * @param  inputByteLen    The length of the input message in bytes.
  * @param  output          Pointer to the output buffer.
  * @param  outputByteLen   The desired number of output bytes.
  * @param  customization   Pointer to the customization string (C).
  * @param  customByteLen   The length of the customization string in bytes.
  * @return 0 if successful, 1 otherwise.
  */
int KangarooTwelve(const unsigned char *input, size_t inputByteLen, unsigned char *output, size_t outputByteLen, const unsigned char *customization, size_t customByteLen );

/**
  * Function to initialize a KangarooTwelve instance.
  * @param  ktInstance      Pointer to the instance to be initialized.
  * @param  outputByteLen   The desired number of output bytes,
  *                         or 0 for an arbitrarily-long output.
  * @return 0 if successful, 1 otherwise.
  */
int KangarooTwelve_Initialize(KangarooTwelve_Instance *ktInstance, size_t outputByteLen);

/**
  * Function to give input data to be absorbed.
  * @param  ktInstance      Pointer to the instance initialized by KangarooTwelve_Initialize().
  * @param  input           Pointer to the input message data (M).
  * @param  inputByteLen    The number of bytes provided in the input message data.
  * @return 0 if successful, 1 otherwise.
  */
int KangarooTwelve_Update(KangarooTwelve_Instance *ktInstance, const unsigned char *input, size_t inputByteLen);

/**
  * Function to call after all the input message has been input, and to get
  * output bytes if the length was specified when calling KangarooTwelve_Initialize().
  * @param  ktInstance      Pointer to the hash instance initialized by KangarooTwelve_Initialize().
  * If @a outputByteLen was not 0 in the call to KangarooTwelve_Initialize(), the number of
  *     output bytes is equal to @a outputByteLen.
  * If @a outputByteLen was 0 in the call to KangarooTwelve_Initialize(), the output bytes
  *     must be extracted using the KangarooTwelve_Squeeze() function.
  * @param  output          Pointer to the buffer where to store the output data.
  * @param  customization   Pointer to the customization string (C).
  * @param  customByteLen   The length of the customization string in bytes.
  * @return 0 if successful, 1 otherwise.
  */
int KangarooTwelve_Final(KangarooTwelve_Instance *ktInstance, unsigned char *output, const unsigned char *customization, size_t customByteLen);

/**
  * Function to squeeze output data.
  * @param  ktInstance     Pointer to the hash instance initialized by KangarooTwelve_Initialize().
  * @param  data           Pointer to the buffer where to store the output data.
  * @param  outputByteLen  The number of output bytes desired.
  * @pre    KangarooTwelve_Final() must have been already called.
  * @return 0 if successful, 1 otherwise.
  */
int KangarooTwelve_Squeeze(KangarooTwelve_Instance *ktInstance, unsigned char *output, size_t outputByteLen);

#endif

#endif
