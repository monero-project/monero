(use-modules ((gnu packages base) #:select (coreutils-minimal findutils tar))
             ((gnu packages bash) #:select (bash-minimal))
             ((gnu packages compression) #:select (gzip))
             ((gnu packages curl) #:select (curl))
             ((gnu packages nss) #:select (nss-certs))
             ((gnu packages rust) #:select (rust)))

(packages->manifest
  (append
    (list
      ;; The Basics
      bash-minimal
      coreutils-minimal

      ;; File(system) inspection
      findutils ;; find

      ;; Cargo
      (list rust "cargo")
      curl ;; networking
      nss-certs ;; idem

      ;; Compression
      gzip
      tar)))
