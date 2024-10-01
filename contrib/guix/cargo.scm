(use-modules (gnu packages)
             (gnu packages bash)
             ((gnu packages certs) #:select (nss-certs))
             (gnu packages rust)
             (gnu packages compression)
             (gnu packages wget)
             (gnu packages gawk)
             ((gnu packages tls) #:select (openssl)))

(packages->manifest
  (append
    (list
      bash
      coreutils-minimal
      findutils ;; find
      nss-certs
      openssl
      tar
      gzip
      wget
      sed
      gawk
      grep
      (list rust "cargo"))))