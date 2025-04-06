(use-modules (gnu packages)
             ((gnu packages bash) #:select (bash-minimal))
             ((gnu packages certs) #:select (nss-certs))
             (gnu packages compression)
             (gnu packages curl)
             (gnu packages moreutils)
             (gnu packages rust)
             ((gnu packages tls) #:select (openssl))
             ((gnu packages web) #:select (jq))
             (guix build-system trivial)
             (guix download)
             ((guix licenses) #:prefix license:)
             (guix packages))

;; We don't use (list rust "rust-src") for two reasons:
;;
;; - Hashes in Cargo.lock are replaced, resulting in:
;;   error: checksum for `<package> <version>` changed between lock files
;;
;; - It drags in a bunch of unnecessary deps, including python.
;;   See: guix graph --type=references rust | xdot -

;; Instead, we create a new package with the unaltered rust source
;; and vendor the standard library in cargo.sh

(define-public rust-std
  (package
    (name "rust-std")
    (version (package-version rust))
    ;; You'd expect (source (package-source (rust)) to work here,
    ;; but it refers to the source store item and NOT the .tar.gz archive
    (source (origin
              (method url-fetch)
              (uri (origin-uri (package-source rust)))
              (sha256
                (content-hash-value (origin-hash (package-source rust))))))
    (build-system trivial-build-system)
    (native-inputs (list tar gzip))
    (arguments
      `(#:modules ((guix build utils))
         #:builder
         (begin
           (use-modules (guix build utils))
           (let ((out (assoc-ref %outputs "out"))
                  (source (assoc-ref %build-inputs "source"))
                  (tar (search-input-file %build-inputs "/bin/tar"))
                  (gzip (search-input-file %build-inputs "/bin/gzip"))
                  (gzip-path (string-append (assoc-ref %build-inputs "gzip") "/bin")))
             (setenv "PATH" gzip-path)
             (mkdir out)
             (invoke tar "xvf" source "-C" out "--strip-components=1")))))
    (synopsis (package-synopsis rust))
    (description (package-description rust))
    (home-page (package-home-page rust))
    (license (package-license rust))))

(packages->manifest
  (append
    (list
      bash-minimal
      coreutils-minimal
      curl
      findutils ;; find
      grep
      gzip
      jq
      nss-certs
      openssl
      sed
      tar
      (list rust "cargo")
      rust-std)))
