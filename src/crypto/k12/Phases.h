/*
Implementation by Ronny Van Keer, hereby denoted as "the implementer".

For more information, feedback or questions, please refer to our website:
https://keccak.team/

To the extent possible under law, the implementer has waived all copyright
and related or neighboring rights to the source code in this file.
http://creativecommons.org/publicdomain/zero/1.0/
*/

#ifndef _Phases_h_
#define _Phases_h_

typedef enum {
    NOT_INITIALIZED,
    ABSORBING,
    FINAL,
    SQUEEZING
} KCP_Phases;

#endif
