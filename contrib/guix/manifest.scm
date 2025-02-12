(use-modules (gnu packages)
             (gnu packages autotools)
             (gnu packages bash)
             ((gnu packages cmake) #:select (cmake-minimal))
             (gnu packages commencement)
             (gnu packages compression)
             (gnu packages cross-base)
             ((gnu packages elf) #:select (patchelf))
             (gnu packages file)
             (gnu packages gawk)
             (gnu packages gcc)
             (gnu packages gperf)
             ((gnu packages libusb) #:select (libplist))
             ((gnu packages linux) #:select (linux-libre-headers-6.1 util-linux))
             (gnu packages llvm)
             (gnu packages mingw)
             (gnu packages moreutils)
             (gnu packages perl)
             (gnu packages pkg-config)
             ((gnu packages python) #:select (python-minimal))
             ((gnu packages tls) #:select (openssl))
             ((gnu packages version-control) #:select (git-minimal))
             (guix build-system gnu)
             (guix build-system trivial)
             (guix download)
             (guix gexp)
             (guix git-download)
             ((guix licenses) #:prefix license:)
             (guix packages)
             ((guix utils) #:select (substitute-keyword-arguments)))

(define-syntax-rule (search-our-patches file-name ...)
  "Return the list of absolute file names corresponding to each
FILE-NAME found in ./patches relative to the current file."
  (parameterize
      ((%patch-path (list (string-append (dirname (current-filename)) "/patches"))))
    (list (search-patch file-name) ...)))

(define building-on (string-append "--build=" (list-ref (string-split (%current-system) #\-) 0) "-guix-linux-gnu"))

(define (make-cross-toolchain target
                              base-gcc-for-libc
                              base-kernel-headers
                              base-libc
                              base-gcc)
  "Create a cross-compilation toolchain package for TARGET"
  (let* ((xbinutils (cross-binutils target))
         ;; 1. Build a cross-compiling gcc without targeting any libc, derived
         ;; from BASE-GCC-FOR-LIBC
         (xgcc-sans-libc (cross-gcc target
                                    #:xgcc base-gcc-for-libc
                                    #:xbinutils xbinutils))
         ;; 2. Build cross-compiled kernel headers with XGCC-SANS-LIBC, derived
         ;; from BASE-KERNEL-HEADERS
         (xkernel (cross-kernel-headers target
                                        #:linux-headers base-kernel-headers
                                        #:xgcc xgcc-sans-libc
                                        #:xbinutils xbinutils))
         ;; 3. Build a cross-compiled libc with XGCC-SANS-LIBC and XKERNEL,
         ;; derived from BASE-LIBC
         (xlibc (cross-libc target
                            #:libc base-libc
                            #:xgcc xgcc-sans-libc
                            #:xbinutils xbinutils
                            #:xheaders xkernel))
         ;; 4. Build a cross-compiling gcc targeting XLIBC, derived from
         ;; BASE-GCC
         (xgcc (cross-gcc target
                          #:xgcc base-gcc
                          #:xbinutils xbinutils
                          #:libc xlibc)))
    ;; Define a meta-package that propagates the resulting XBINUTILS, XLIBC, and
    ;; XGCC
    (package
      (name (string-append target "-toolchain"))
      (version (package-version xgcc))
      (source #f)
      (build-system trivial-build-system)
      (arguments '(#:builder (begin (mkdir %output) #t)))
      (propagated-inputs
        (list xbinutils
              xlibc
              xgcc
              `(,xlibc "static")
              `(,xgcc "lib")))
      (synopsis (string-append "Complete GCC tool chain for " target))
      (description (string-append "This package provides a complete GCC tool
chain for " target " development."))
      (home-page (package-home-page xgcc))
      (license (package-license xgcc)))))

(define base-gcc gcc-12)
(define base-linux-kernel-headers linux-libre-headers-6.1)

(define* (make-monero-cross-toolchain  target
                                       #:key
                                       (base-gcc-for-libc linux-base-gcc)
                                       (base-kernel-headers base-linux-kernel-headers)
                                       (base-libc glibc-2.27)
                                       (base-gcc linux-base-gcc))
  "Convenience wrapper around MAKE-CROSS-TOOLCHAIN with default values
desirable for building Monero release binaries."
  (make-cross-toolchain target
                        base-gcc-for-libc
                        base-kernel-headers
                        base-libc
                        base-gcc))

(define (gcc-mingw-patches gcc)
  (package-with-extra-patches gcc
    (search-our-patches "gcc-remap-guix-store.patch")))

(define (make-mingw-pthreads-cross-toolchain target)
  "Create a cross-compilation toolchain package for TARGET"
  (let* ((xbinutils (cross-binutils target))
         (pthreads-xlibc (package-with-extra-patches (cond ((string-prefix? "i686-" target)
                                                       mingw-w64-i686-winpthreads)
                                                 (else mingw-w64-x86_64-winpthreads))
                           (search-our-patches "winpthreads-remap-guix-store.patch")))
         (pthreads-xgcc (cross-gcc target
                                    #:xgcc (gcc-mingw-patches mingw-w64-base-gcc)
                                    #:xbinutils xbinutils
                                    #:libc pthreads-xlibc)))
    ;; Define a meta-package that propagates the resulting XBINUTILS, XLIBC, and
    ;; XGCC
    (package
      (name (string-append target "-posix-toolchain"))
      (version (package-version pthreads-xgcc))
      (source #f)
      (build-system trivial-build-system)
      (arguments '(#:builder (begin (mkdir %output) #t)))
      (propagated-inputs
        (list xbinutils
              pthreads-xlibc
              pthreads-xgcc
              `(,pthreads-xgcc "lib")))
      (synopsis (string-append "Complete GCC tool chain for " target))
      (description (string-append "This package provides a complete GCC tool
chain for " target " development."))
      (home-page (package-home-page pthreads-xgcc))
      (license (package-license pthreads-xgcc)))))

(define-public mingw-w64-base-gcc
  (package
    (inherit base-gcc)
    (arguments
      (substitute-keyword-arguments (package-arguments base-gcc)
        ((#:configure-flags flags)
          `(append ,flags
            ;; https://gcc.gnu.org/install/configure.html
            (list "--enable-threads=posix",
                  building-on)))))))

(define-public linux-base-gcc
  (package
    (inherit (package-with-extra-patches base-gcc
               (search-our-patches "gcc-remap-guix-store.patch")))
    (arguments
      (substitute-keyword-arguments (package-arguments base-gcc)
        ((#:configure-flags flags)
          `(append ,flags
            ;; https://gcc.gnu.org/install/configure.html
            (list "--enable-initfini-array=yes",
                  "--enable-default-ssp=yes",
                  "--enable-default-pie=yes",
                  "--enable-standard-branch-protection=yes",
                  "--enable-cet=yes",
                  building-on)))
        ((#:phases phases)
          `(modify-phases ,phases
            ;; Given a XGCC package, return a modified package that replace each instance of
            ;; -rpath in the default system spec that's inserted by Guix with -rpath-link
            (add-after 'pre-configure 'replace-rpath-with-rpath-link
             (lambda _
               (substitute* (cons "gcc/config/rs6000/sysv4.h"
                                  (find-files "gcc/config"
                                              "^gnu-user.*\\.h$"))
                 (("-rpath=") "-rpath-link="))
               #t))))))))

(define-public glibc-2.27
  (package
    (inherit glibc-2.31)
    (version "2.27")
    (source (origin
              (method git-fetch)
              (uri (git-reference
                    (url "https://sourceware.org/git/glibc.git")
                    (commit "73886db6218e613bd6d4edf529f11e008a6c2fa6")))
              (file-name (git-file-name "glibc" "73886db6218e613bd6d4edf529f11e008a6c2fa6"))
              (sha256
               (base32
                "0azpb9cvnbv25zg8019rqz48h8i2257ngyjg566dlnp74ivrs9vq"))
              (patches (search-our-patches "glibc-2.27-riscv64-Use-__has_include-to-include-asm-syscalls.h.patch"
                                           "glibc-2.27-guix-prefix.patch"
                                           "glibc-2.27-no-librt.patch"
                                           "glibc-2.27-riscv64-fix-incorrect-jal-with-HIDDEN_JUMPTARGET.patch"))))
    (arguments
      (substitute-keyword-arguments (package-arguments glibc)
        ((#:configure-flags flags)
          `(append ,flags
            ;; https://www.gnu.org/software/libc/manual/html_node/Configuring-and-compiling.html
            (list "--enable-stack-protector=all",
                  "--enable-bind-now",
                  "--disable-werror",
                  building-on)))
    ((#:phases phases)
        `(modify-phases ,phases
           (add-before 'configure 'set-etc-rpc-installation-directory
             (lambda* (#:key outputs #:allow-other-keys)
               ;; Install the rpc data base file under `$out/etc/rpc'.
               ;; Otherwise build will fail with "Permission denied."
               (let ((out (assoc-ref outputs "out")))
                 (substitute* "sunrpc/Makefile"
                   (("^\\$\\(inst_sysconfdir\\)/rpc(.*)$" _ suffix)
                    (string-append out "/etc/rpc" suffix "\n"))
                   (("^install-others =.*$")
                    (string-append "install-others = " out "/etc/rpc\n"))))))))))
    (native-inputs
      (modify-inputs (package-native-inputs glibc-2.31)
        (delete "make")
        (append gnu-make-4.2))))) ;; make >= 4.4 causes an infinite loop (stdio-common)


; This list declares which packages are included in the container environment. It
; should reflect the minimal set of packages we need to build and debug the build
; process. Guix will also include the run-time dependencies for each package.
;
; If a package is target-specific, place it in the corresponding list at the end.
; Be mindful when adding new packages here. Some packages take a very long time
; to bootstrap. Prefer -minimal versions of packages, unless there is a good
; reason not to.
;
; To show run-time dependencies, run:
; $ guix time-machine --commit=<pinned commit> -- graph --type=references <package> | xdot -
;
; To show build-time dependencies (excluding bootstrap), run:
; $ guix time-machine --commit=<pinned commit> -- graph <package> | xdot -

(packages->manifest
 (append
  (list ;; The Basics
        bash
        ; the build graph for bash-minimal is slightly smaller.
        ; however, it does not include readline support which
        ; makes debugging inside the guix container inconvenient
        coreutils-minimal
        ; includes basic shell utilities: cat, cp, echo, mkdir, etc
        which

        ;; File(system) inspection
        file
        grep
        diffutils ; provides diff
        findutils ; provides find and xargs

        ;; File transformation
        patch
        gawk
        sed
        patchelf  ; unused, occassionally useful for debugging

        ;; Compression and archiving
        tar
        bzip2 ; used to create release archives (non-windows)
        gzip  ; used to unpack most packages in depends
        xz    ; used to unpack freebsd_base
        zip   ; used to create release archives (windows)
        unzip ; used to unpack android_ndk

        ;; Build tools
        gnu-make
        libtool
        autoconf-2.71 ; defaults to 2.69, which does not recognize the aarch64-apple-darwin target
        automake
        pkg-config
        cmake-minimal

        ;; Scripting
        perl           ; required to build openssl in depends
        python-minimal ; required to build monero (cmake/CheckTrezor.cmake) and in android_ndk

        ;; Git
        git-minimal ; used to create the release source archive
    )
  (let ((target (getenv "HOST")))
    (cond ((string-suffix? "-mingw32" target)
           (list
             gcc-toolchain-12
             (make-mingw-pthreads-cross-toolchain target)))
          ((string-contains target "-linux-gnu")
           (list
             gcc-toolchain-12
             (list gcc-toolchain-12 "static")
             (make-monero-cross-toolchain target)))
          ((string-contains target "freebsd")
           (list
             gcc-toolchain-12
             (list gcc-toolchain-12 "static")
             clang-toolchain-11 binutils))
          ((string-contains target "android")
            (list
              gcc-toolchain-12
              (list gcc-toolchain-12 "static")))
          ((string-contains target "darwin")
           (list
             gcc-toolchain-10
             clang-toolchain-11
             binutils))
          (else '())))))
