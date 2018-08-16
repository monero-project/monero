/**
 * Copyright (c) 2018 m2049r
 * <p>
 * Licensed under the Apache License, Version 2.0 (the "License");
 * you may not use this file except in compliance with the License.
 * You may obtain a copy of the License at
 * <p>
 * http://www.apache.org/licenses/LICENSE-2.0
 * <p>
 * Unless required by applicable law or agreed to in writing, software
 * distributed under the License is distributed on an "AS IS" BASIS,
 * WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
 * See the License for the specific language governing permissions and
 * limitations under the License.
 */

#ifndef XMRWALLET_LEDGER_H
#define XMRWALLET_LEDGER_H

#ifdef __cplusplus
extern "C"
{
#endif

#define SCARD_S_SUCCESS ((LONG)0x00000000) /**< No error was encountered. */
#define SCARD_E_INSUFFICIENT_BUFFER    ((LONG)0x80100008) /**< The data buffer to receive returned data is too small for the returned data. */
#define SCARD_E_NO_READERS_AVAILABLE ((LONG)0x8010002E) /**< Cannot find a smart card reader. */

typedef long LONG;
typedef unsigned long DWORD;
typedef DWORD *LPDWORD;
typedef unsigned char BYTE;
typedef BYTE *LPBYTE;
typedef const BYTE *LPCBYTE;

typedef char CHAR;
typedef CHAR *LPSTR;

typedef LONG SCARDCONTEXT;
typedef LONG SCARDHANDLE;

/**
 * @brief LedgerFind - find Ledger Device and return it's name
 * @param buffer - buffer for name of found device
 * @param len    - length of buffer
 * @return  0 - success
 *         -1 - no device connected / found
 *         -2 - JVM not found
 */
int LedgerFind(char *buffer, size_t len);

/**
 * @brief LedgerExchange - exchange data with Ledger Device
 * @param pbSendBuffer   - buffer for data to send
 * @param cbSendLength   - length of send buffer
 * @param pbRecvBuffer   - buffer for received data
 * @param pcbRecvLength  - pointer to size of receive buffer
 *                         gets set with length of received data on successful return
 * @return SCARD_S_SUCCESS - success
 *         SCARD_E_NO_READERS_AVAILABLE - no device connected / found
 *         SCARD_E_INSUFFICIENT_BUFFER  - pbRecvBuffer is too small for the response
 */
LONG LedgerExchange(LPCBYTE pbSendBuffer, DWORD cbSendLength, LPBYTE pbRecvBuffer, LPDWORD pcbRecvLength);

#ifdef __cplusplus
}
#endif

#endif //XMRWALLET_LEDGER_H
