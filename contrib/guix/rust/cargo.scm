(use-modules (gnu packages)
             ((gnu packages bash) #:select (bash-minimal))
             ((gnu packages nss) #:select (nss-certs))
             (gnu packages compression)
             (gnu packages curl)
             (gnu packages moreutils)
             (gnu packages rust)
             ((gnu packages tls) #:select (openssl))
             (guix packages))

(packages->manifest
  (append
    (list
      bash-minimal
      coreutils-minimal
      curl
      findutils ;; find
      grep
      gzip
      nss-certs
      openssl
      tar
      (list rust "cargo"))))
