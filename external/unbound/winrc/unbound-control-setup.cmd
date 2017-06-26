@Echo off
rem
rem unbound-control-setup.cmd - set up SSL certificates for unbound-control
rem
rem Copyright (c) 2008, NLnet Labs. All rights reserved.
rem Modified for Windows by Y.Voinov (c) 2014
rem
rem This software is open source.
rem 
rem Redistribution and use in source and binary forms, with or without
rem modification, are permitted provided that the following conditions
rem are met:
rem 
rem Redistributions of source code must retain the above copyright notice,
rem this list of conditions and the following disclaimer.
rem 
rem Redistributions in binary form must reproduce the above copyright notice,
rem this list of conditions and the following disclaimer in the documentation
rem and/or other materials provided with the distribution.
rem 
rem Neither the name of the NLNET LABS nor the names of its contributors may
rem be used to endorse or promote products derived from this software without
rem specific prior written permission.
rem 
rem THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS
rem "AS IS" AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT
rem LIMITED TO, THE IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR
rem A PARTICULAR PURPOSE ARE DISCLAIMED. IN NO EVENT SHALL THE COPYRIGHT
rem HOLDER OR CONTRIBUTORS BE LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL,
rem SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT LIMITED
rem TO, PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES; LOSS OF USE, DATA, OR
rem PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND ON ANY THEORY OF
rem LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT (INCLUDING
rem NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE OF THIS
rem SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.

rem settings:

rem directory for files
set prefix="C:\Program Files"
set DESTDIR=%prefix%\Unbound

rem issuer and subject name for certificates
set SERVERNAME=unbound
set CLIENTNAME=unbound-control

rem validity period for certificates
set DAYS=7200

rem size of keys in bits
set BITS=1536

rem hash algorithm
set HASH=sha256

rem base name for unbound server keys
set SVR_BASE=unbound_server

rem base name for unbound-control keys
set CTL_BASE=unbound_control

rem end of options

rem Check OpenSSL installed
for /f "delims=" %%a in ('where openssl') do @set SSL_PROGRAM=%%a
if /I "%SSL_PROGRAM%"=="" echo SSL not found. If installed, add path to PATH environment variable. & exit 1
echo SSL found: %SSL_PROGRAM%

set arg=%1
if /I "%arg%" == "-h" goto help
if /I "%arg%"=="-d" set DESTDIR=%2

rem go!:
echo setup in directory %DESTDIR%
cd %DESTDIR%

rem create certificate keys; do not recreate if they already exist.
if exist %SVR_BASE%.key (
echo %SVR_BASE%.key exists
goto next
)
echo generating %SVR_BASE%.key
"%SSL_PROGRAM%" genrsa -out %SVR_BASE%.key %BITS% || echo could not genrsa && exit 1

:next
if exist %CTL_BASE%.key (
echo %CTL_BASE%.key exists
goto next2
)
echo generating %CTL_BASE%.key
"%SSL_PROGRAM%" genrsa -out %CTL_BASE%.key %BITS% || echo could not genrsa && exit 1

:next2
rem create self-signed cert for server
if exist request.cfg (del /F /Q /S request.cfg)
echo [req]>>request.cfg
echo default_bits=%BITS%>>request.cfg
echo default_md=%HASH%>>request.cfg
echo prompt=no>>request.cfg
echo distinguished_name=req_distinguished_name>>request.cfg
echo.>>request.cfg
echo [req_distinguished_name]>>request.cfg
echo commonName=%SERVERNAME%>>request.cfg

if not exist request.cfg (
echo could not create request.cfg
exit 1
)

echo create %SVR_BASE%.pem (self signed certificate)
"%SSL_PROGRAM%" req -key %SVR_BASE%.key -config request.cfg  -new -x509 -days %DAYS% -out %SVR_BASE%.pem || echo could not create %SVR_BASE%.pem && exit 1
rem create trusted usage pem
"%SSL_PROGRAM%" x509 -in %SVR_BASE%.pem -addtrust serverAuth -out %SVR_BASE%_trust.pem

rem create client request and sign it
if exist request.cfg (del /F /Q /S request.cfg)
echo [req]>>request.cfg
echo default_bits=%BITS%>>request.cfg
echo default_md=%HASH%>>request.cfg
echo prompt=no>>request.cfg
echo distinguished_name=req_distinguished_name>>request.cfg
echo.>>request.cfg
echo [req_distinguished_name]>>request.cfg
echo commonName=%CLIENTNAME%>>request.cfg

if not exist request.cfg (
echo could not create request.cfg
exit 1
)

echo create %CTL_BASE%.pem (signed client certificate)
"%SSL_PROGRAM%" req -key %CTL_BASE%.key -config request.cfg -new | "%SSL_PROGRAM%" x509 -req -days %DAYS% -CA %SVR_BASE%_trust.pem -CAkey %SVR_BASE%.key -CAcreateserial -%HASH% -out %CTL_BASE%.pem

if not exist %CTL_BASE%.pem (
echo could not create %CTL_BASE%.pem
exit 1
)
rem create trusted usage pem
rem "%SSL_PROGRAM%" x509 -in %CTL_BASE%.pem -addtrust clientAuth -out %CTL_BASE%_trust.pem

rem see details with "%SSL_PROGRAM%" x509 -noout -text < %SVR_BASE%.pem
rem echo "create %CTL_BASE%_browser.pfx (web client certificate)"
rem echo "create webbrowser PKCSrem12 .PFX certificate file. In Firefox import in:"
rem echo "preferences - advanced - encryption - view certificates - your certs"
rem echo "empty password is used, simply click OK on the password dialog box."
rem "%SSL_PROGRAM%" pkcs12 -export -in %CTL_BASE%_trust.pem -inkey %CTL_BASE%.key -name "unbound remote control client cert" -out %CTL_BASE%_browser.pfx -password "pass:" || echo could not create browser certificate && exit 1

rem remove crap
del /F /Q /S request.cfg
del /F /Q /S %CTL_BASE%_trust.pem 
del /F /Q /S %SVR_BASE%_trust.pem 
del /F /Q /S %SVR_BASE%_trust.srl

echo Setup success. Certificates created. Enable in unbound.conf file to use

exit 0

:help
echo unbound-control-setup.cmd - setup SSL keys for unbound-control
echo 	-d dir	use directory to store keys and certificates.
echo 		default: %DESTDIR%
echo please run this command using the same user id that the 
echo unbound daemon uses, it needs read privileges.
exit 1
