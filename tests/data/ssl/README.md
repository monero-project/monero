# TLS unit-test credentials

`test.key` and `test.crt` are public, test-only credentials for loopback TLS tests.
Never use this private key outside tests. The tests disable peer verification;
they exercise connection and record handling, not certificate trust or validity.

Loading these fixtures avoids generating a new RSA key for each client and server
context. They were generated with:

```sh
openssl req -x509 -newkey rsa:4096 -sha256 -nodes \
  -keyout test.key -out test.crt -days 36500 \
  -subj '/CN=monero-unit-tests.invalid'
```
