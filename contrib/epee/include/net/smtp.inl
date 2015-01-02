// Copyright (c) 2006-2013, Andrey N. Sabelnikov, www.sabelnikov.net
// All rights reserved.
// 
// Redistribution and use in source and binary forms, with or without
// modification, are permitted provided that the following conditions are met:
// * Redistributions of source code must retain the above copyright
// notice, this list of conditions and the following disclaimer.
// * Redistributions in binary form must reproduce the above copyright
// notice, this list of conditions and the following disclaimer in the
// documentation and/or other materials provided with the distribution.
// * Neither the name of the Andrey N. Sabelnikov nor the
// names of its contributors may be used to endorse or promote products
// derived from this software without specific prior written permission.
// 
// THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS "AS IS" AND
// ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE IMPLIED
// WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE ARE
// DISCLAIMED. IN NO EVENT SHALL THE COPYRIGHT OWNER  BE LIABLE FOR ANY
// DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES
// (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES;
// LOSS OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND
// ON ANY THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT
// (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE OF THIS
// SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
// 



#include "md5.h"

namespace epee
{
namespace net_utils
{
	namespace smtp
	{


		//////////////////////////////////////////////////////////////////////////
		inline char * convert_hex( unsigned char *in, int len )
		{
			static char hex[] = "0123456789abcdef";
			char * out;
			int i;

			out = (char *) malloc(len * 2 + 1);
			if (out == NULL)
				return NULL;

			for (i = 0; i < len; i++) {
				out[i * 2] = hex[in[i] >> 4];
				out[i * 2 + 1] = hex[in[i] & 15];
			}

			out[i*2] = 0;

			return out;
		}

		//////////////////////////////////////////////////////////////////////////
		inline char * hash_md5(const char * sec_key, const char * data, int len)
		{
			char key[65], digest[24];
			char * hash_hex;

			int sec_len, i;

			sec_len = strlen(sec_key);

			if (sec_len < 64) {
				memcpy(key, sec_key, sec_len);
				for (i = sec_len; i < 64; i++) {
					key[i] = 0;
				}
			} else {
				memcpy(key, sec_key, 64);
			}

			md5::hmac_md5( (const unsigned char*)data, len, (const unsigned char*)key, 64, (unsigned char*)digest );
			hash_hex = convert_hex( (unsigned char*)digest, 16 );

			return hash_hex;
		}
		//////////////////////////////////////////////////////////////////////////
		//////////////////////////////////////////////////////////////////////////
		//////////////////////////////////////////////////////////////////////////
		//////////////////////////////////////////////////////////////////////////
		inline CSMTPClient::CSMTPClient(void)
		{
			m_dwSupportedAuthModesCount = 0;
			m_bConnected = FALSE;
			m_hSocket = INVALID_SOCKET;
			m_pErrorText = NULL;

			// Initialize WinSock
			WORD wVer  = MAKEWORD( 2, 2 );    
			if ( WSAStartup( wVer, &m_wsaData ) != NO_ERROR )
			{
				SetErrorText( "WSAStartup.", WSAGetLastError() );        
				throw; 
			}
			if ( LOBYTE( m_wsaData.wVersion ) != 2 || HIBYTE( m_wsaData.wVersion ) != 2  )
			{
				SetErrorText( "Can't find a useable WinSock DLL." );
				WSACleanup();
				throw; 
			}    
		}

		//////////////////////////////////////////////////////////////////////////
		inline CSMTPClient::~CSMTPClient(void)
		{
			if ( m_pErrorText )
			{
				free( m_pErrorText );
				m_pErrorText = NULL;
			}

			if ( m_bConnected )
				ServerDisconnect();

			// Cleanup
			WSACleanup();
		}

		//////////////////////////////////////////////////////////////////////////
		inline void CSMTPClient::SetErrorText( LPCSTR szErrorText, DWORD dwErrorCode )
		{
			if ( m_pErrorText )
			{
				free( m_pErrorText );
				m_pErrorText = NULL;
			}

			LPVOID lpMsgBuf = NULL;
			if ( dwErrorCode )
			{
				FormatMessageA(
					FORMAT_MESSAGE_ALLOCATE_BUFFER | 
					FORMAT_MESSAGE_FROM_SYSTEM,
					NULL,
					dwErrorCode,
					MAKELANGID(LANG_NEUTRAL, SUBLANG_DEFAULT),
					(LPSTR) &lpMsgBuf,
					0, NULL );
			}

			if ( szErrorText && strlen( szErrorText ) )
			{
				m_pErrorText = (LPBYTE)malloc( strlen( szErrorText ) + 1 );
				strcpy( (char*)m_pErrorText, szErrorText );

				if ( lpMsgBuf )
				{
					strcat( (char*)m_pErrorText, " " );
					strcpy( (char*)m_pErrorText, (char*)lpMsgBuf );

					LocalFree( lpMsgBuf );
				}
			}
		}

		inline void CSMTPClient::SetErrorText( PBYTE szErrorText, DWORD dwErrorCode )
		{
			SetErrorText( (LPCSTR)szErrorText, dwErrorCode );
		}

		//////////////////////////////////////////////////////////////////////////
		inline char* CSMTPClient::GetLastErrorText()
		{
			return (char*)m_pErrorText;
		}

		//////////////////////////////////////////////////////////////////////////
		inline DWORD CSMTPClient::ReceiveData( SOCKET hSocket, PBYTE pReceiveBuffer, DWORD dwReceiveBufferSize )
		{
			DWORD dwReceivedDataSize = 0;

			if ( hSocket != INVALID_SOCKET && pReceiveBuffer && dwReceiveBufferSize )
			{
				int iReceived = 0;
				int iLength = 0;

				iLength = recv( hSocket, (LPSTR)pReceiveBuffer + iReceived, dwReceiveBufferSize - iReceived, 
					NO_FLAGS );

				if ( iLength != 0 && iLength != SOCKET_ERROR )
					iReceived += iLength;

				dwReceivedDataSize = iReceived;

				pReceiveBuffer[ iReceived ] = 0;
			}

			return dwReceivedDataSize;
		}

		inline //////////////////////////////////////////////////////////////////////////
		DWORD CSMTPClient::SendData( SOCKET hSocket, PBYTE pSendBuffer, DWORD dwSendBufferSize )
		{
			DWORD dwSended = 0;

			if ( hSocket != INVALID_SOCKET && pSendBuffer && dwSendBufferSize )
			{
				int iSended = 0;
				int iLength = 0;

				while ( iLength != SOCKET_ERROR && dwSendBufferSize - iSended > 0 )
				{
					iLength = send( hSocket, (LPSTR)pSendBuffer + iSended, dwSendBufferSize - iSended, 
						NO_FLAGS );

					if ( iLength != 0 && iLength != SOCKET_ERROR )
						iSended += iLength;
				}

				dwSended = iSended;
			}

			//if ( dwSended )
			//	printf( "C: %s", pSendBuffer );

			return dwSended;
		}

		//////////////////////////////////////////////////////////////////////////
		inline unsigned short CSMTPClient::GetResponseCode( LPBYTE pBuffer, DWORD dwBufferSize ) 
		{
			unsigned short iCode = 0;

			if ( dwBufferSize >= 3 )
			{
				CHAR szResponseCode[ 4 ] = { 0 };
				memcpy( szResponseCode, pBuffer, 3 );
				szResponseCode[ 3 ] = 0;
				iCode = atoi( szResponseCode );
			}

			return iCode;
		}

		//////////////////////////////////////////////////////////////////////////
		inline void CSMTPClient::ParseESMTPExtensions( LPBYTE pBuffer, DWORD dwBufferSize )
		{
			const char *szSubstring = strstr( (const char*)pBuffer, "250-AUTH " );
			if ( !szSubstring )
			{
				szSubstring = strstr( (const char*)pBuffer, "250 AUTH " );
			}

			if ( szSubstring )
			{
				const char *szSubstringEnd = strstr( (const char*)szSubstring, "\r\n" );
				if ( szSubstringEnd )
				{
					szSubstring += 9;
					char szAuthMode[ 256 ] = { 0 };
					for ( ; szSubstring < szSubstringEnd + 1 ; szSubstring++ )
					{
						if ( *szSubstring == ' ' || *szSubstring == '\r' )
						{
							if ( _strcmpi( szAuthMode, SMTP_COMMAND_AUTH_PLAIN ) == 0 )
							{
								m_aSupportedAuthModes[ m_dwSupportedAuthModesCount ] = AUTH_MODE_PLAIN;
								m_dwSupportedAuthModesCount++;
							}
							else if ( _strcmpi( szAuthMode, SMTP_COMMAND_AUTH_LOGIN ) == 0 )
							{
								m_aSupportedAuthModes[ m_dwSupportedAuthModesCount ] = AUTH_MODE_LOGIN;
								m_dwSupportedAuthModesCount++;
							}
							else if ( _strcmpi( szAuthMode, SMTP_COMMAND_AUTH_CRAM_MD5 ) == 0 )
							{
								m_aSupportedAuthModes[ m_dwSupportedAuthModesCount ] = AUTH_MODE_CRAM_MD5;
								m_dwSupportedAuthModesCount++;
							}

							szAuthMode[ 0 ] = 0;

							if ( m_dwSupportedAuthModesCount == MAX_AUTH_MODES_COUND )
								break;
						}
						else
						{
							szAuthMode[ strlen( szAuthMode ) + 1 ] = 0;
							szAuthMode[ strlen( szAuthMode ) ] = *szSubstring;
						}
					}
				}
			}
		}

		//////////////////////////////////////////////////////////////////////////
		inline BOOL CSMTPClient::ServerConnect( LPCSTR szServerAddress, const unsigned short iPortNumber )
		{
			if ( m_bConnected )
				ServerDisconnect();

			m_bConnected = FALSE;
			m_hSocket = INVALID_SOCKET;

			m_hSocket = _connectServerSocket( szServerAddress, iPortNumber );  

			if ( m_hSocket != INVALID_SOCKET )
			{
				DWORD dwReceiveBufferSize = 1024*16;
				PBYTE pReceiveBuffer = (PBYTE)malloc( dwReceiveBufferSize );
				if ( pReceiveBuffer )
				{
					// Connected. Wait server hello string.
					DWORD iReceived = ReceiveData( m_hSocket, pReceiveBuffer, dwReceiveBufferSize );
					if ( iReceived )
					{
						// Check 220
						int iResponseCode = GetResponseCode( pReceiveBuffer, iReceived );
						if ( iResponseCode != 220 )
						{
							SetErrorText( pReceiveBuffer );
							free( pReceiveBuffer );
							ServerDisconnect();
							return FALSE;
						}
					}
					else
					{
						SetErrorText( "ReceiveData error. ", WSAGetLastError() );
						free( pReceiveBuffer );
						ServerDisconnect();
						return FALSE;
					}

					// EHLO / HELO
					BYTE szHelloBuffer[ 256 ];
					sprintf( (char*)szHelloBuffer, "%s %s\r\n", (char*)SMTP_COMMAND_EHLO, (char*)szServerAddress );
					if ( SendData( m_hSocket, (PBYTE)szHelloBuffer, strlen( (const char*)szHelloBuffer ) ) == 0 )
					{
						SetErrorText( "SendData error.", WSAGetLastError() );    
						free( pReceiveBuffer );
						ServerDisconnect();
						return FALSE;
					}

					iReceived = ReceiveData( m_hSocket, pReceiveBuffer, dwReceiveBufferSize );
					if ( iReceived )
					{
						// Check 250
						int iResponseCode = GetResponseCode( pReceiveBuffer, iReceived );
						if ( iResponseCode == 500 )
						{
							SetErrorText( pReceiveBuffer );

							sprintf( (char*)szHelloBuffer, "%s %s\r\n", (char*)SMTP_COMMAND_HELO, (char*)szServerAddress );
							if ( SendData( m_hSocket, (PBYTE)szHelloBuffer, strlen( (const char*)szHelloBuffer ) ) == 0 )
							{
								SetErrorText( "SendData error.", WSAGetLastError() );    
								free( pReceiveBuffer );
								ServerDisconnect();
								return FALSE;
							}

							iResponseCode = GetResponseCode( pReceiveBuffer, iReceived );
							if ( iResponseCode != 250 )
							{
								SetErrorText( pReceiveBuffer );
								free( pReceiveBuffer );
								ServerDisconnect();
								return FALSE;
							}
						}
						else if ( iResponseCode != 250 )
						{
							SetErrorText( pReceiveBuffer );
							free( pReceiveBuffer );
							ServerDisconnect();
							return FALSE;
						}

						// Parse AUTH supported modes
						ParseESMTPExtensions( pReceiveBuffer, iReceived );
					}
					else
					{
						SetErrorText( "ReceiveData error.", WSAGetLastError() );
						free( pReceiveBuffer );
						ServerDisconnect();
						return FALSE;
					}

					free( pReceiveBuffer );
				}
			}
			else
			{
				return FALSE;
			}

			m_bConnected = TRUE;

			return TRUE;
		}

		//////////////////////////////////////////////////////////////////////////
		inline BOOL CSMTPClient::ServerConnect( LPCSTR szServerAddress, const unsigned short iPortNumber, LPCSTR szUsername, LPCSTR szPassword )
		{
			BOOL bSuccess = FALSE;

			bSuccess = ServerConnect( szServerAddress, iPortNumber );
			if ( bSuccess )
			{
				if ( GetAuthModeIsSupported( AUTH_MODE_CRAM_MD5 ) )
				{
					ServerLogin( szUsername, szPassword, AUTH_MODE_CRAM_MD5 );
				}
				else
					if ( GetAuthModeIsSupported( AUTH_MODE_PLAIN ) )
					{
						ServerLogin( szUsername, szPassword, AUTH_MODE_PLAIN );
					}
					else
						if ( GetAuthModeIsSupported( AUTH_MODE_LOGIN ) )
						{
							ServerLogin( szUsername, szPassword, AUTH_MODE_LOGIN );
						}
			}

			return bSuccess;
		}

		//////////////////////////////////////////////////////////////////////////
		inline SOCKET CSMTPClient::_connectServerSocket( LPCSTR szServerAddress, const unsigned short iPortNumber )
		{
			int              nConnect;
			short            nProtocolPort  = iPortNumber;
			LPHOSTENT        lpHostEnt;
			SOCKADDR_IN      sockAddr;        

			SOCKET           hServerSocket = INVALID_SOCKET;

			lpHostEnt = gethostbyname( szServerAddress );
			if (lpHostEnt)
			{        
				hServerSocket = socket( AF_INET, SOCK_STREAM, IPPROTO_TCP );
				if (hServerSocket != INVALID_SOCKET)
				{
					sockAddr.sin_family = AF_INET;
					sockAddr.sin_port = htons( nProtocolPort );
					sockAddr.sin_addr = *((LPIN_ADDR)*lpHostEnt->h_addr_list);

					nConnect = connect( hServerSocket, (PSOCKADDR)&sockAddr, 
						sizeof(sockAddr) );

					if ( nConnect != 0 ) 
					{
						SetErrorText( "connect error.", WSAGetLastError() );    
						hServerSocket = INVALID_SOCKET;
					}
				} 
				else
				{
					SetErrorText( "Invalid socket." );
					throw;
				}
			}
			else
			{
				SetErrorText( "Error retrieving host by name.", WSAGetLastError() );
			}

			return hServerSocket ;
		}

		//////////////////////////////////////////////////////////////////////////
		inline void CSMTPClient::ServerDisconnect()
		{
			if ( m_hSocket != INVALID_SOCKET )
			{
				if ( SendData( m_hSocket, (PBYTE)SMTP_COMMAND_QUIT, strlen( SMTP_COMMAND_QUIT ) ) == 0 )
				{
					SetErrorText( "SendData error.", WSAGetLastError() );    
					return;
				}

				DWORD dwReceiveBufferSize = 1024*16;
				PBYTE pReceiveBuffer = (PBYTE)malloc( dwReceiveBufferSize );
				if ( pReceiveBuffer )
				{
					DWORD iReceived = ReceiveData( m_hSocket, pReceiveBuffer, dwReceiveBufferSize );

					if ( iReceived )
						SetErrorText( pReceiveBuffer );    

					free( pReceiveBuffer );
				}

				m_hSocket = INVALID_SOCKET;
			}

			m_bConnected = FALSE;
		}

		//////////////////////////////////////////////////////////////////////////
		inline BOOL CSMTPClient::GetAuthModeIsSupported( int iMode )
		{
			BOOL bSupported = FALSE;

			for ( int i = 0 ; i < m_dwSupportedAuthModesCount ; i++ )
			{
				if ( m_aSupportedAuthModes[ i ] == iMode )
				{
					bSupported = TRUE;
					break;
				}
			}

			return bSupported;
		}

		//////////////////////////////////////////////////////////////////////////
		inline BOOL CSMTPClient::ServerLogin( LPCSTR szUsername, LPCSTR szPassword, int iAuthMode )
		{
			BOOL bSuccess = FALSE;

			if ( iAuthMode == AUTH_MODE_PLAIN )
			{
				bSuccess = ServerLoginMethodPlain( szUsername, szPassword );
			}
			else if ( iAuthMode == AUTH_MODE_LOGIN )
			{
				bSuccess = ServerLoginMethodLogin( szUsername, szPassword );
			}
			else if ( iAuthMode == AUTH_MODE_CRAM_MD5 )
			{
				bSuccess = ServerLoginMethodCramMD5( szUsername, szPassword );
			}

			return bSuccess;
		}

		//////////////////////////////////////////////////////////////////////////
		inline BOOL CSMTPClient::ServerLogin( LPCSTR szUsername, LPCSTR szPassword )
		{
			BOOL bSuccess = FALSE;

			if ( GetAuthModeIsSupported( AUTH_MODE_CRAM_MD5 ) )
			{
				bSuccess = ServerLogin( szUsername, szPassword, AUTH_MODE_CRAM_MD5 );
			}
			else
				if ( GetAuthModeIsSupported( AUTH_MODE_PLAIN ) )
				{
					bSuccess = ServerLogin( szUsername, szPassword, AUTH_MODE_PLAIN );
				}
				else
					if ( GetAuthModeIsSupported( AUTH_MODE_LOGIN ) )
					{
						bSuccess = ServerLogin( szUsername, szPassword, AUTH_MODE_LOGIN );
					}

					return bSuccess;
		}

		//////////////////////////////////////////////////////////////////////////
		inline BOOL CSMTPClient::ServerLoginMethodPlain( LPCSTR szUsername, LPCSTR szPassword )
		{
			BOOL bSuccess = FALSE;

			BYTE szCommandBuffer[ 256 ];
			sprintf( (char*)szCommandBuffer, "%s %s\r\n", (char*)SMTP_COMMAND_AUTH, (char*)SMTP_COMMAND_AUTH_PLAIN );
			if ( SendData( m_hSocket, (PBYTE)szCommandBuffer, strlen( (const char*)szCommandBuffer ) ) == 0 )
			{
				SetErrorText( "SendData error.", WSAGetLastError() );    
				return FALSE;
			}

			DWORD dwReceiveBufferSize = 1024*16;
			PBYTE pReceiveBuffer = (PBYTE)malloc( dwReceiveBufferSize );
			if ( pReceiveBuffer )
			{
				// Connected. Wait server hello string.
				DWORD iReceived = ReceiveData( m_hSocket, pReceiveBuffer, dwReceiveBufferSize );
				if ( iReceived )
				{
					SetErrorText( pReceiveBuffer );

					// Check 334
					int iResponseCode = GetResponseCode( pReceiveBuffer, iReceived );
					if ( iResponseCode != 334 )
					{
						free( pReceiveBuffer );
						return FALSE;
					}
				}
				else
				{
					SetErrorText( "ReceiveData error.", WSAGetLastError() );    
					free( pReceiveBuffer );
					return FALSE;
				}

				// Encode.
				DWORD dwLoginBuffer = strlen( szUsername ) + strlen( szPassword ) + 3;
				char *pLoginBuffer = (char*)malloc( dwLoginBuffer );
				if ( pLoginBuffer )
				{
					ZeroMemory( pLoginBuffer, dwLoginBuffer );
					strcpy( pLoginBuffer + 1, szUsername );
					strcpy( pLoginBuffer + 1 + strlen( szUsername ) + 1, szPassword );

					Base64Coder coder;
					coder.Encode( (const PBYTE)pLoginBuffer, dwLoginBuffer - 1 );
					LPCSTR szLoginBufferEncoded = coder.EncodedMessage();

					if ( szLoginBufferEncoded && strlen( szLoginBufferEncoded ) > 0 )
					{
						DWORD dwSendBufferSize = strlen( szLoginBufferEncoded ) + 4;
						char* pSendBuffer = (char*)malloc( dwSendBufferSize );
						if ( pSendBuffer )
						{
							strcpy( pSendBuffer, szLoginBufferEncoded );
							strcat( pSendBuffer, "\r\n" );

							if ( SendData( m_hSocket, (PBYTE)pSendBuffer, strlen( (const char*)pSendBuffer ) ) == 0 )
							{
								SetErrorText( "SendData error.", WSAGetLastError() );    
								free( pSendBuffer );
								free( pLoginBuffer );
								free( pReceiveBuffer );
								return FALSE;
							}

							free( pSendBuffer );
						}
					}

					free( pLoginBuffer );

					// check result
					iReceived = ReceiveData( m_hSocket, pReceiveBuffer, dwReceiveBufferSize );
					if ( iReceived )
					{
						SetErrorText( pReceiveBuffer );

						// Check 235
						int iResponseCode = GetResponseCode( pReceiveBuffer, iReceived );
						if ( iResponseCode != 235 )
						{
							free( pReceiveBuffer );
							return FALSE;
						}

						bSuccess = TRUE;
					}
					else
					{
						SetErrorText( "ReceiveData error.", WSAGetLastError() );    
						free( pReceiveBuffer );
						return FALSE;
					}
				}

				free( pReceiveBuffer );
			}

			return bSuccess;
		}

		//////////////////////////////////////////////////////////////////////////
		inline BOOL CSMTPClient::ServerLoginMethodLogin( LPCSTR szUsername, LPCSTR szPassword )
		{
			BOOL bSuccess = FALSE;

			BYTE szCommandBuffer[ 256 ];
			sprintf( (char*)szCommandBuffer, "%s %s\r\n", (char*)SMTP_COMMAND_AUTH, (char*)SMTP_COMMAND_AUTH_LOGIN );
			if ( SendData( m_hSocket, (PBYTE)szCommandBuffer, strlen( (const char*)szCommandBuffer ) ) == 0 )
			{
				SetErrorText( "SendData error.", WSAGetLastError() );    
				return FALSE;
			}

			DWORD dwReceiveBufferSize = 1024*16;
			PBYTE pReceiveBuffer = (PBYTE)malloc( dwReceiveBufferSize );
			if ( pReceiveBuffer )
			{
				DWORD iReceived = ReceiveData( m_hSocket, pReceiveBuffer, dwReceiveBufferSize );
				if ( iReceived )
				{
					SetErrorText( pReceiveBuffer );    

					// Check 334
					int iResponseCode = GetResponseCode( pReceiveBuffer, iReceived );
					if ( iResponseCode != 334 )
					{
						free( pReceiveBuffer );
						return FALSE;
					}

					// Check request
					if ( iReceived > 6 )
					{
						Base64Coder coder;
						coder.Decode( pReceiveBuffer + 4, iReceived - 6 );
						LPCSTR szRequest = coder.DecodedMessage();
						if ( szRequest && strlen( szRequest ) > 0 )
						{
							if ( strcmpi( szRequest, "Username:" ) == 0 )
							{
								coder.Encode( (const PBYTE)szUsername, strlen( szUsername ) );
								LPCSTR szUsernameEncoded = coder.EncodedMessage();

								char* szLoginUsernameBuffer = (char*)malloc( strlen( szUsernameEncoded ) + 4 );
								if ( szLoginUsernameBuffer )
								{
									strcpy( szLoginUsernameBuffer, szUsernameEncoded );
									strcat( szLoginUsernameBuffer, "\r\n" );

									if ( SendData( m_hSocket, (PBYTE)szLoginUsernameBuffer, strlen( (const char*)szLoginUsernameBuffer ) ) == 0 )
									{
										SetErrorText( "SendData error.", WSAGetLastError() );    
										free( pReceiveBuffer );
										return FALSE;
									}

									free( szLoginUsernameBuffer );
								}
								else
								{
									free( pReceiveBuffer );
									return FALSE;
								}

								iReceived = ReceiveData( m_hSocket, pReceiveBuffer, dwReceiveBufferSize );
								if ( iReceived )
								{
									SetErrorText( pReceiveBuffer );

									// Check 334
									int iResponseCode = GetResponseCode( pReceiveBuffer, iReceived );
									if ( iResponseCode != 334 )
									{
										free( pReceiveBuffer );
										return FALSE;
									}

									// Check request
									if ( iReceived > 6 )
									{
										coder.Decode( pReceiveBuffer + 4, iReceived - 6 );
										LPCSTR szRequest2 = coder.DecodedMessage();
										if ( szRequest2 && strlen( szRequest2 ) > 0 )
										{
											if ( strcmpi( szRequest2, "Password:" ) == 0 )
											{
												coder.Encode( (const PBYTE)szPassword, strlen( szPassword ) );
												LPCSTR szPasswordEncoded = coder.EncodedMessage();

												char* szLoginPasswordBuffer = (char*)malloc( strlen( szPasswordEncoded ) + 4 );
												if ( szLoginPasswordBuffer )
												{
													strcpy( szLoginPasswordBuffer, szPasswordEncoded );
													strcat( szLoginPasswordBuffer, "\r\n" );

													if ( SendData( m_hSocket, (PBYTE)szLoginPasswordBuffer, strlen( (const char*)szLoginPasswordBuffer ) ) == 0 )
													{
														SetErrorText( "SendData error.", WSAGetLastError() );    
														free( pReceiveBuffer );
														return FALSE;
													}

													free( szLoginPasswordBuffer );
												}
												else
												{
													free( pReceiveBuffer );
													return FALSE;
												}

												iReceived = ReceiveData( m_hSocket, pReceiveBuffer, dwReceiveBufferSize );
												if ( iReceived )
												{
													SetErrorText( pReceiveBuffer );

													// Check 235
													int iResponseCode = GetResponseCode( pReceiveBuffer, iReceived );
													if ( iResponseCode != 235 )
													{
														free( pReceiveBuffer );
														return FALSE;
													}

													bSuccess = TRUE;
												}
												else
												{
													SetErrorText( "ReceiveData error.", WSAGetLastError() );    
													free( pReceiveBuffer );
													return FALSE;
												}
											}
										}
									}
								}
								else
								{
									free( pReceiveBuffer );
									return FALSE;
								}
							}
						}
						else
						{
							free( pReceiveBuffer );
							return FALSE;
						}
					}
					else
					{
						free( pReceiveBuffer );
						return FALSE;
					}
				}
				else
				{
					SetErrorText( "ReceiveData error.", WSAGetLastError() );    
					free( pReceiveBuffer );
					return FALSE;
				}

				free( pReceiveBuffer );
			}

			return bSuccess;
		}

		//////////////////////////////////////////////////////////////////////////
		inline BOOL CSMTPClient::ServerLoginMethodCramMD5( LPCSTR szUsername, LPCSTR szPassword )
		{
			BOOL bSuccess = FALSE;

			BYTE szCommandBuffer[ 256 ];
			sprintf( (char*)szCommandBuffer, "%s %s\r\n", (char*)SMTP_COMMAND_AUTH, (char*)SMTP_COMMAND_AUTH_CRAM_MD5 );
			if ( SendData( m_hSocket, (PBYTE)szCommandBuffer, strlen( (const char*)szCommandBuffer ) ) == 0 )
			{
				SetErrorText( "SendData error.", WSAGetLastError() );    
				return FALSE;
			}

			DWORD dwReceiveBufferSize = 1024*16;
			PBYTE pReceiveBuffer = (PBYTE)malloc( dwReceiveBufferSize );
			if ( pReceiveBuffer )
			{
				// Connected. Wait server hello string.
				DWORD iReceived = ReceiveData( m_hSocket, pReceiveBuffer, dwReceiveBufferSize );
				if ( iReceived )
				{
					SetErrorText( pReceiveBuffer );

					// Check 334
					int iResponseCode = GetResponseCode( pReceiveBuffer, iReceived );
					if ( iResponseCode != 334 )
					{
						free( pReceiveBuffer );
						return FALSE;
					}

					// Check request
					if ( iReceived > 6 )
					{
						Base64Coder coder;
						coder.Decode( pReceiveBuffer + 4, iReceived - 6 );
						LPCSTR szResponse = coder.DecodedMessage();
						if ( szResponse && strlen( szResponse ) > 0 )
						{
							char *auth_hex = hash_md5( szPassword, szResponse, strlen(szResponse) );
							if ( !auth_hex )
							{
								free( pReceiveBuffer );
								return FALSE;
							}

							char *szCommand = (char*)malloc( strlen( szUsername ) + strlen( auth_hex ) + 5 );
							if ( szCommand )
							{
								sprintf( szCommand, "%s %s", szUsername, auth_hex );

								free( auth_hex );

								coder.Encode( (const PBYTE)szCommand, strlen( szCommand ) );

								free( szCommand );

								LPCSTR szAuthEncoded = coder.EncodedMessage();
								if ( szAuthEncoded == NULL )
								{
									free( pReceiveBuffer );
									return FALSE;
								}

								char *szAuthCommand = (char*)malloc( strlen( szAuthEncoded ) + 4 );
								if ( szAuthCommand )
								{
									strcpy( szAuthCommand, szAuthEncoded );
									strcat( szAuthCommand, "\r\n" );

									// Send auth data
									if ( SendData( m_hSocket, (PBYTE)szAuthCommand, strlen( (const char*)szAuthCommand ) ) == 0 )
									{
										SetErrorText( "SendData error.", WSAGetLastError() );    
										free( szAuthCommand );
										free( pReceiveBuffer );
										return FALSE;
									}

									// Check response
									iReceived = ReceiveData( m_hSocket, pReceiveBuffer, dwReceiveBufferSize );
									if ( iReceived )
									{
										SetErrorText( pReceiveBuffer );

										// Check 235
										int iResponseCode = GetResponseCode( pReceiveBuffer, iReceived );
										if ( iResponseCode != 235 )
										{
											free( pReceiveBuffer );
											return FALSE;
										}

										bSuccess = TRUE;
									}
									else
									{
										SetErrorText( "ReceiveData error.", WSAGetLastError() );    
										free( pReceiveBuffer );
										return FALSE;
									}

									free( szAuthCommand );
								}
								else
								{
									free( pReceiveBuffer );
									return FALSE;
								}
							}
							else
							{
								free( auth_hex );
								free( pReceiveBuffer );
								return FALSE;
							}
						}
						else
						{
							free( pReceiveBuffer );
							return FALSE;
						}
					}

				}
				else
				{
					SetErrorText( "ReceiveData error.", WSAGetLastError() );    
					free( pReceiveBuffer );
					return FALSE;
				}

				free( pReceiveBuffer );
			}
			else
			{
				SetErrorText( "malloc() failed.", GetLastError() );    
			}

			return bSuccess;
		}

		//////////////////////////////////////////////////////////////////////////
		inline BOOL CSMTPClient::SendMessage( LPCSTR szFromAddress, LPCSTR szFromName, LPCSTR szToAddresses, LPCSTR szSubject, LPCSTR szXMailer, LPBYTE pBodyBuffer, DWORD dwBodySize )
		{
			BOOL bSuccess = FALSE;

			// Format Header
			if ( !szFromAddress )
			{
				SetErrorText( "SendMessage. Invalid Parameters!" );
				return NULL;
			}

			char *szHeaderBuffer = (char*)malloc( 1024 * 16 );
			if ( szHeaderBuffer )
			{
				// get the current date and time
				char szDate[ 500 ];
				char sztTime[ 500 ];

				SYSTEMTIME st = { 0 };
				::GetSystemTime(&st);

				::GetDateFormatA( MAKELCID( MAKELANGID( LANG_ENGLISH, SUBLANG_ENGLISH_US), SORT_DEFAULT), 0, &st, "ddd',' dd MMM yyyy", szDate , sizeof( szDate ) );
				::GetTimeFormatA( MAKELCID( MAKELANGID( LANG_ENGLISH, SUBLANG_ENGLISH_US), SORT_DEFAULT), TIME_FORCE24HOURFORMAT, &st, "HH':'mm':'ss", sztTime, sizeof( sztTime ) );

				sprintf( szHeaderBuffer, "DATE: %s %s\r\n", szDate, sztTime );

				// X-Mailer Field
				if ( szXMailer && strlen( szXMailer ) )
				{
					strcat( szHeaderBuffer, "X-Mailer: " );
					strcat( szHeaderBuffer, szXMailer );
					strcat( szHeaderBuffer, "\r\n" );
				}

				// From:
				strcat( szHeaderBuffer, "From: " );
				if ( szFromName )
				{
					strcat( szHeaderBuffer, "\"" );
					strcat( szHeaderBuffer, szFromName );
					strcat( szHeaderBuffer, "\" <" );
					strcat( szHeaderBuffer, szFromAddress );
					strcat( szHeaderBuffer, ">\r\n" );
				}
				else
				{
					strcat( szHeaderBuffer, "<" );
					strcat( szHeaderBuffer, szFromAddress );
					strcat( szHeaderBuffer, ">\r\n" );
				}

				// Subject:
				if ( szSubject && strlen( szSubject ) )
				{
					strcat( szHeaderBuffer, "Subject: " );
					strcat( szHeaderBuffer, szSubject );
					strcat( szHeaderBuffer, "\r\n" );
				}

				// To Fields
				strcat( szHeaderBuffer, "To: " );
				strcat( szHeaderBuffer, szToAddresses );
				strcat( szHeaderBuffer, "\r\n" );

				// MIME
				strcat( szHeaderBuffer, "MIME-Version: 1.0\r\nContent-type: text/plain; charset=US-ASCII\r\n" );

				// End Header
				strcat( szHeaderBuffer, "\r\n" );
			}
			else
			{
				SetErrorText( "malloc error.", GetLastError() );
				return FALSE;
			}


			BYTE szCommandBuffer[ 256 ];
			sprintf( (char*)szCommandBuffer, "MAIL FROM:<%s> SIZE=%u\r\n", (char*)szFromAddress, strlen( szHeaderBuffer ) + dwBodySize + 2 );
			if ( SendData( m_hSocket, (PBYTE)szCommandBuffer, strlen( (const char*)szCommandBuffer ) ) == 0 )
			{
				SetErrorText( "SendData error.", WSAGetLastError() );    
				free( szHeaderBuffer );
				return FALSE;
			}

			DWORD dwReceiveBufferSize = 1024*16;
			PBYTE pReceiveBuffer = (PBYTE)malloc( dwReceiveBufferSize );
			if ( pReceiveBuffer )
			{
				DWORD iReceived = ReceiveData( m_hSocket, pReceiveBuffer, dwReceiveBufferSize );
				if ( iReceived )
				{
					SetErrorText( pReceiveBuffer );

					// Check 250
					int iResponseCode = GetResponseCode( pReceiveBuffer, iReceived );
					if ( iResponseCode != 250 )
					{
						free( szHeaderBuffer );
						free( pReceiveBuffer );
						return FALSE;
					}
				}
				else
				{
					SetErrorText( "ReceiveData error.", WSAGetLastError() );    
					free( szHeaderBuffer );
					free( pReceiveBuffer );
					return FALSE;
				}

				// Post "RCTP TO:"
				char *szCurrentAddr = (char*)malloc( strlen( szToAddresses ) + 1 );
				if ( !szCurrentAddr )
				{
					SetErrorText( "malloc error.", GetLastError() );    
					free( szHeaderBuffer );
					free( pReceiveBuffer );
					return FALSE;
				}

				const char* szToOffset = szToAddresses;
				char* szZap = NULL;

				BOOL bRCPTAccepted = FALSE;
				do 
				{
					strcpy( szCurrentAddr, szToOffset );
					char *szExtractedAdress = szCurrentAddr;
					szZap = strchr( szCurrentAddr, ',' );

					if ( szZap )
					{
						*szZap = 0;
						szToOffset = szZap + 1;
					}

					char *pSkobka1 = strchr( szCurrentAddr, '<' );
					char *pSkobka2 = strchr( szCurrentAddr, '>' );

					if ( pSkobka1 && pSkobka2 && pSkobka2 > pSkobka1 )
					{
						szExtractedAdress = pSkobka1 + 1;
						*pSkobka2 = NULL;
					}

					if ( szExtractedAdress && strlen( szExtractedAdress ) > 0 )
					{
						sprintf( (char*)szCommandBuffer, "RCPT TO:<%s>\r\n", (char*)szExtractedAdress );
						if ( SendData( m_hSocket, (PBYTE)szCommandBuffer, strlen( (const char*)szCommandBuffer ) ) == 0 )
						{
							SetErrorText( "SendData error.", WSAGetLastError() );    
							free( szCurrentAddr );
							free( pReceiveBuffer );
							free( szHeaderBuffer );
							return FALSE;
						}

						iReceived = ReceiveData( m_hSocket, pReceiveBuffer, dwReceiveBufferSize );
						if ( iReceived )
						{
							SetErrorText( pReceiveBuffer );

							// Check 250
							int iResponseCode = GetResponseCode( pReceiveBuffer, iReceived );
							if ( iResponseCode == 250 )
							{
								bRCPTAccepted = TRUE;
							}
						}
						else
						{
							SetErrorText( "ReceiveData error.", WSAGetLastError() );    
							free( szCurrentAddr );
							free( pReceiveBuffer );
							free( szHeaderBuffer );
							return FALSE;
						}
					}

				} while( szZap );

				free( szCurrentAddr );

				if ( bRCPTAccepted )
				{
					sprintf( (char*)szCommandBuffer, "DATA\r\n" );
					if ( SendData( m_hSocket, (PBYTE)szCommandBuffer, strlen( (const char*)szCommandBuffer ) ) == 0 )
					{
						SetErrorText( "SendData error.", WSAGetLastError() );    
						free( pReceiveBuffer );
						free( szHeaderBuffer );
						return FALSE;
					}

					iReceived = ReceiveData( m_hSocket, pReceiveBuffer, dwReceiveBufferSize );
					if ( iReceived )
					{
						SetErrorText( pReceiveBuffer );

						// Check 354
						int iResponseCode = GetResponseCode( pReceiveBuffer, iReceived );
						if ( iResponseCode != 354 )
						{
							free( pReceiveBuffer );
							free( szHeaderBuffer );
							return FALSE;
						}
					}
					else
					{
						SetErrorText( "ReceiveData error.", WSAGetLastError() );    
						free( pReceiveBuffer );
						free( szHeaderBuffer );
						return FALSE;
					}

					// Send message data (header + body + .)
					if ( SendData( m_hSocket, (PBYTE)szHeaderBuffer, strlen( (const char*)szHeaderBuffer ) ) == 0 )
					{
						SetErrorText( "SendData error.", WSAGetLastError() );    
						free( pReceiveBuffer );
						free( szHeaderBuffer );
						return FALSE;
					}

					if ( SendData( m_hSocket, (PBYTE)pBodyBuffer, dwBodySize ) == 0 )
					{
						SetErrorText( "SendData error.", WSAGetLastError() );    
						free( pReceiveBuffer );
						free( szHeaderBuffer );
						return FALSE;
					}

					if ( SendData( m_hSocket, (PBYTE)"\r\n.\r\n", 5 ) == 0 )
					{
						SetErrorText( "SendData error.", WSAGetLastError() );    
						free( pReceiveBuffer );
						free( szHeaderBuffer );
						return FALSE;
					}

					iReceived = ReceiveData( m_hSocket, pReceiveBuffer, dwReceiveBufferSize );
					if ( iReceived )
					{
						SetErrorText( pReceiveBuffer );

						// Check 250
						int iResponseCode = GetResponseCode( pReceiveBuffer, iReceived );
						if ( iResponseCode == 250 )
						{
							bSuccess = TRUE;
						}
					}
					else
					{
						SetErrorText( "ReceiveData error.", WSAGetLastError() );    
					}
				}

				free( pReceiveBuffer );
			}
			else
			{
				SetErrorText( "malloc error.", GetLastError() );
			}

			if ( szHeaderBuffer )
				free( szHeaderBuffer );

			return bSuccess;
		}



		//////////////////////////////////////////////////////////////////////////
		//////////////////////////////////////////////////////////////////////////
		//////////////////////////////////////////////////////////////////////////
		//////////////////////////////////////////////////////////////////////////


#ifndef PAGESIZE
#define PAGESIZE					4096
#endif

#ifndef ROUNDTOPAGE
#define ROUNDTOPAGE(a)			(((a/4096)+1)*4096)
#endif

		//////////////////////////////////////////////////////////////////////
		// Construction/Destruction
		//////////////////////////////////////////////////////////////////////

		inline Base64Coder::Base64Coder()
			:	m_pDBuffer(NULL),
			m_pEBuffer(NULL),
			m_nDBufLen(0),
			m_nEBufLen(0)
		{

		}

		inline Base64Coder::~Base64Coder()
		{
			if(m_pDBuffer != NULL)
				delete [] m_pDBuffer;

			if(m_pEBuffer != NULL)
				delete [] m_pEBuffer;
		}

		inline LPCSTR Base64Coder::DecodedMessage() const 
		{ 
			return (LPCSTR) m_pDBuffer;
		}

		inline LPCSTR Base64Coder::EncodedMessage() const
		{ 
			return (LPCSTR) m_pEBuffer;
		}

		inline void Base64Coder::AllocEncode(DWORD nSize)
		{
			if(m_nEBufLen < nSize)
			{
				if(m_pEBuffer != NULL)
					delete [] m_pEBuffer;

				m_nEBufLen = ROUNDTOPAGE(nSize);
				m_pEBuffer = new BYTE[m_nEBufLen];
			}

			::ZeroMemory(m_pEBuffer, m_nEBufLen);
			m_nEDataLen = 0;
		}

		inline void Base64Coder::AllocDecode(DWORD nSize)
		{
			if(m_nDBufLen < nSize)
			{
				if(m_pDBuffer != NULL)
					delete [] m_pDBuffer;

				m_nDBufLen = ROUNDTOPAGE(nSize);
				m_pDBuffer = new BYTE[m_nDBufLen];
			}

			::ZeroMemory(m_pDBuffer, m_nDBufLen);
			m_nDDataLen = 0;
		}

		inline void Base64Coder::SetEncodeBuffer(const PBYTE pBuffer, DWORD nBufLen)
		{
			DWORD	i = 0;

			AllocEncode(nBufLen);
			while(i < nBufLen)
			{
				if(!_IsBadMimeChar(pBuffer[i]))
				{
					m_pEBuffer[m_nEDataLen] = pBuffer[i];
					m_nEDataLen++;
				}

				i++;
			}
		}

		inline void Base64Coder::SetDecodeBuffer(const PBYTE pBuffer, DWORD nBufLen)
		{
			AllocDecode(nBufLen);
			::CopyMemory(m_pDBuffer, pBuffer, nBufLen);
			m_nDDataLen = nBufLen;
		}

		inline void Base64Coder::Encode(const PBYTE pBuffer, DWORD nBufLen)
		{
			SetDecodeBuffer(pBuffer, nBufLen);
			AllocEncode(nBufLen * 2);

			TempBucket			Raw;
			DWORD					nIndex	= 0;

			while((nIndex + 3) <= nBufLen)
			{
				Raw.Clear();
				::CopyMemory(&Raw, m_pDBuffer + nIndex, 3);
				Raw.nSize = 3;
				_EncodeToBuffer(Raw, m_pEBuffer + m_nEDataLen);
				nIndex		+= 3;
				m_nEDataLen	+= 4;
			}

			if(nBufLen > nIndex)
			{
				Raw.Clear();
				Raw.nSize = (BYTE) (nBufLen - nIndex);
				::CopyMemory(&Raw, m_pDBuffer + nIndex, nBufLen - nIndex);
				_EncodeToBuffer(Raw, m_pEBuffer + m_nEDataLen);
				m_nEDataLen += 4;
			}
		}

		inline void Base64Coder::Encode(LPCSTR szMessage)
		{
			if(szMessage != NULL)
				Base64Coder::Encode((const PBYTE)szMessage, strlen( (const char*)szMessage));
		}

		inline void Base64Coder::Decode(const PBYTE pBuffer, DWORD dwBufLen)
		{
			if(is_init())
				_Init();

			SetEncodeBuffer(pBuffer, dwBufLen);

			AllocDecode(dwBufLen);

			TempBucket			Raw;

			DWORD		nIndex = 0;

			while((nIndex + 4) <= m_nEDataLen)
			{
				Raw.Clear();
				Raw.nData[0] = DecodeTable()[m_pEBuffer[nIndex]];
				Raw.nData[1] = DecodeTable()[m_pEBuffer[nIndex + 1]];
				Raw.nData[2] = DecodeTable()[m_pEBuffer[nIndex + 2]];
				Raw.nData[3] = DecodeTable()[m_pEBuffer[nIndex + 3]];

				if(Raw.nData[2] == 255)
					Raw.nData[2] = 0;
				if(Raw.nData[3] == 255)
					Raw.nData[3] = 0;

				Raw.nSize = 4;
				_DecodeToBuffer(Raw, m_pDBuffer + m_nDDataLen);
				nIndex += 4;
				m_nDDataLen += 3;
			}

			// If nIndex < m_nEDataLen, then we got a decode message without padding.
			// We may want to throw some kind of warning here, but we are still required
			// to handle the decoding as if it was properly padded.
			if(nIndex < m_nEDataLen)
			{
				Raw.Clear();
				for(DWORD i = nIndex; i < m_nEDataLen; i++)
				{
					Raw.nData[i - nIndex] = DecodeTable()[m_pEBuffer[i]];
					Raw.nSize++;
					if(Raw.nData[i - nIndex] == 255)
						Raw.nData[i - nIndex] = 0;
				}

				_DecodeToBuffer(Raw, m_pDBuffer + m_nDDataLen);
				m_nDDataLen += (m_nEDataLen - nIndex);
			}
		}

		inline void Base64Coder::Decode(LPCSTR szMessage)
		{
			if(szMessage != NULL)
				Base64Coder::Decode((const PBYTE)szMessage, strlen((const char*)szMessage));
		}

		inline DWORD Base64Coder::_DecodeToBuffer(const TempBucket &Decode, PBYTE pBuffer)
		{
			TempBucket	Data;
			DWORD			nCount = 0;

			_DecodeRaw(Data, Decode);

			for(int i = 0; i < 3; i++)
			{
				pBuffer[i] = Data.nData[i];
				if(pBuffer[i] != 255)
					nCount++;
			}

			return nCount;
		}


		inline void Base64Coder::_EncodeToBuffer(const TempBucket &Decode, PBYTE pBuffer)
		{
			TempBucket	Data;

			_EncodeRaw(Data, Decode);

			for(int i = 0; i < 4; i++)
				pBuffer[i] = Base64Digits()[Data.nData[i]];

			switch(Decode.nSize)
			{
			case 1:
				pBuffer[2] = '=';
			case 2:
				pBuffer[3] = '=';
			}
		}

		inline void Base64Coder::_DecodeRaw(TempBucket &Data, const TempBucket &Decode)
		{
			BYTE		nTemp;

			Data.nData[0] = Decode.nData[0];
			Data.nData[0] <<= 2;

			nTemp = Decode.nData[1];
			nTemp >>= 4;
			nTemp &= 0x03;
			Data.nData[0] |= nTemp;

			Data.nData[1] = Decode.nData[1];
			Data.nData[1] <<= 4;

			nTemp = Decode.nData[2];
			nTemp >>= 2;
			nTemp &= 0x0F;
			Data.nData[1] |= nTemp;

			Data.nData[2] = Decode.nData[2];
			Data.nData[2] <<= 6;
			nTemp = Decode.nData[3];
			nTemp &= 0x3F;
			Data.nData[2] |= nTemp;
		}

		inline void Base64Coder::_EncodeRaw(TempBucket &Data, const TempBucket &Decode)
		{
			BYTE		nTemp;

			Data.nData[0] = Decode.nData[0];
			Data.nData[0] >>= 2;

			Data.nData[1] = Decode.nData[0];
			Data.nData[1] <<= 4;
			nTemp = Decode.nData[1];
			nTemp >>= 4;
			Data.nData[1] |= nTemp;
			Data.nData[1] &= 0x3F;

			Data.nData[2] = Decode.nData[1];
			Data.nData[2] <<= 2;

			nTemp = Decode.nData[2];
			nTemp >>= 6;

			Data.nData[2] |= nTemp;
			Data.nData[2] &= 0x3F;

			Data.nData[3] = Decode.nData[2];
			Data.nData[3] &= 0x3F;
		}

		inline BOOL Base64Coder::_IsBadMimeChar(BYTE nData)
		{
			switch(nData)
			{
			case '\r': case '\n': case '\t': case ' ' :
			case '\b': case '\a': case '\f': case '\v':
				return TRUE;
			default:
				return FALSE;
			}
		}

		inline void Base64Coder::_Init()
		{  // Initialize Decoding table.

			int	i;

			for(i = 0; i < 256; i++)
				DecodeTable()[i] = -2;

			for(i = 0; i < 64; i++)
			{
				DecodeTable()[Base64Digits()[i]]			= i;
				DecodeTable()[Base64Digits()[i]|0x80]	= i;
			}

			DecodeTable()['=']				= -1;
			DecodeTable()['='|0x80]		= -1;

			is_init() = TRUE;
		}


	}
}
}
