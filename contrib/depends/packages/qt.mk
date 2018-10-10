PACKAGE=qt
$(package)_version=5.7.1
$(package)_download_path=http://download.qt.io/official_releases/qt/5.7/$($(package)_version)/submodules
$(package)_suffix=opensource-src-$($(package)_version).tar.gz
$(package)_file_name=qtbase-$($(package)_suffix)
$(package)_sha256_hash=95f83e532d23b3ddbde7973f380ecae1bac13230340557276f75f2e37984e410
$(package)_dependencies=openssl zlib
$(package)_build_subdir=qtbase
$(package)_qt_libs=corelib
$(package)_patches=pidlist_absolute.patch fix_qt_pkgconfig.patch qfixed-coretext.patch

$(package)_qttranslations_file_name=qttranslations-$($(package)_suffix)
$(package)_qttranslations_sha256_hash=3a15aebd523c6d89fb97b2d3df866c94149653a26d27a00aac9b6d3020bc5a1d

$(package)_qttools_file_name=qttools-$($(package)_suffix)
$(package)_qttools_sha256_hash=22d67de915cb8cd93e16fdd38fa006224ad9170bd217c2be1e53045a8dd02f0f

$(package)_extra_sources  = $($(package)_qttranslations_file_name)
$(package)_extra_sources += $($(package)_qttools_file_name)

define $(package)_set_vars
$(package)_config_opts_release = -release
$(package)_config_opts_debug = -debug
$(package)_config_opts += -bindir $(build_prefix)/bin
$(package)_config_opts += -c++std c++11
$(package)_config_opts += -confirm-license
$(package)_config_opts += -dbus-runtime
$(package)_config_opts += -no-alsa
$(package)_config_opts += -no-audio-backend
$(package)_config_opts += -no-cups
$(package)_config_opts += -no-egl
$(package)_config_opts += -no-eglfs
$(package)_config_opts += -no-feature-style-windowsmobile
$(package)_config_opts += -no-feature-style-windowsce
$(package)_config_opts += -no-freetype
$(package)_config_opts += -no-gif
$(package)_config_opts += -no-glib
$(package)_config_opts += -no-gstreamer
$(package)_config_opts += -no-icu
$(package)_config_opts += -no-iconv
$(package)_config_opts += -no-kms
$(package)_config_opts += -no-linuxfb
$(package)_config_opts += -no-libudev
$(package)_config_opts += -no-mitshm
$(package)_config_opts += -no-mtdev
$(package)_config_opts += -no-pulseaudio
$(package)_config_opts += -no-openvg
$(package)_config_opts += -no-reduce-relocations
$(package)_config_opts += -no-qml-debug
$(package)_config_opts += -no-sql-db2
$(package)_config_opts += -no-sql-ibase
$(package)_config_opts += -no-sql-oci
$(package)_config_opts += -no-sql-tds
$(package)_config_opts += -no-sql-mysql
$(package)_config_opts += -no-sql-odbc
$(package)_config_opts += -no-sql-psql
$(package)_config_opts += -no-sql-sqlite
$(package)_config_opts += -no-sql-sqlite2
$(package)_config_opts += -no-use-gold-linker
$(package)_config_opts += -no-xinput2
$(package)_config_opts += -no-xrender
$(package)_config_opts += -nomake examples
$(package)_config_opts += -nomake tests
$(package)_config_opts += -opensource
$(package)_config_opts += -openssl-linked
$(package)_config_opts += -optimized-qmake
$(package)_config_opts += -pch
$(package)_config_opts += -pkg-config
$(package)_config_opts += -qt-libpng
$(package)_config_opts += -qt-libjpeg
$(package)_config_opts += -qt-pcre
$(package)_config_opts += -system-zlib
$(package)_config_opts += -reduce-exports
$(package)_config_opts += -static
$(package)_config_opts += -silent
$(package)_config_opts += -v
$(package)_config_opts += -no-feature-printer
$(package)_config_opts += -no-feature-printdialog
$(package)_config_opts += -no-gui
$(package)_config_opts += -no-freetype
$(package)_config_opts += -no-sm
$(package)_config_opts += -no-fontconfig
$(package)_config_opts += -no-opengl
$(package)_config_opts += -no-xkb
$(package)_config_opts += -no-xcb
$(package)_config_opts += -no-xshape
$(package)_build_env  = QT_RCC_TEST=1
endef

define $(package)_fetch_cmds
$(call fetch_file,$(package),$($(package)_download_path),$($(package)_download_file),$($(package)_file_name),$($(package)_sha256_hash)) && \
$(call fetch_file,$(package),$($(package)_download_path),$($(package)_qttranslations_file_name),$($(package)_qttranslations_file_name),$($(package)_qttranslations_sha256_hash)) && \
$(call fetch_file,$(package),$($(package)_download_path),$($(package)_qttools_file_name),$($(package)_qttools_file_name),$($(package)_qttools_sha256_hash))
endef

define $(package)_extract_cmds
  mkdir -p $($(package)_extract_dir) && \
  echo "$($(package)_sha256_hash)  $($(package)_source)" > $($(package)_extract_dir)/.$($(package)_file_name).hash && \
  echo "$($(package)_qttranslations_sha256_hash)  $($(package)_source_dir)/$($(package)_qttranslations_file_name)" >> $($(package)_extract_dir)/.$($(package)_file_name).hash && \
  echo "$($(package)_qttools_sha256_hash)  $($(package)_source_dir)/$($(package)_qttools_file_name)" >> $($(package)_extract_dir)/.$($(package)_file_name).hash && \
  $(build_SHA256SUM) -c $($(package)_extract_dir)/.$($(package)_file_name).hash && \
  mkdir qtbase && \
  tar --strip-components=1 -xf $($(package)_source) -C qtbase && \
  mkdir qttranslations && \
  tar --strip-components=1 -xf $($(package)_source_dir)/$($(package)_qttranslations_file_name) -C qttranslations && \
  mkdir qttools && \
  tar --strip-components=1 -xf $($(package)_source_dir)/$($(package)_qttools_file_name) -C qttools
endef


define $(package)_preprocess_cmds
  sed -i.old "s|updateqm.commands = \$$$$\$$$$LRELEASE|updateqm.commands = $($(package)_extract_dir)/qttools/bin/lrelease|" qttranslations/translations/translations.pro && \
  sed -i.old "/updateqm.depends =/d" qttranslations/translations/translations.pro && \
  patch -p1 < $($(package)_patch_dir)/pidlist_absolute.patch && \
  patch -p1 < $($(package)_patch_dir)/fix_qt_pkgconfig.patch && \
  patch -p1 < $($(package)_patch_dir)/qfixed-coretext.patch && \
  echo "!host_build: QMAKE_CFLAGS     += $($(package)_cflags) $($(package)_cppflags)" >> qtbase/mkspecs/common/gcc-base.conf && \
  echo "!host_build: QMAKE_CXXFLAGS   += $($(package)_cxxflags) $($(package)_cppflags)" >> qtbase/mkspecs/common/gcc-base.conf && \
  echo "!host_build: QMAKE_LFLAGS     += $($(package)_ldflags)" >> qtbase/mkspecs/common/gcc-base.conf
endef

define $(package)_config_cmds
  export PKG_CONFIG_SYSROOT_DIR=/ && \
  export PKG_CONFIG_LIBDIR=$(host_prefix)/lib/pkgconfig && \
  export PKG_CONFIG_PATH=$(host_prefix)/share/pkgconfig  && \
  ./configure $($(package)_config_opts) && \
  echo "host_build: QT_CONFIG ~= s/system-zlib/zlib" >> mkspecs/qconfig.pri && \
  echo "CONFIG += force_bootstrap" >> mkspecs/qconfig.pri && \
  $(MAKE) sub-src-clean && \
  cd ../qttranslations && ../qtbase/bin/qmake qttranslations.pro -o Makefile && \
  cd translations && ../../qtbase/bin/qmake translations.pro -o Makefile && cd ../.. &&\
  cd qttools/src/linguist/lrelease/ && ../../../../qtbase/bin/qmake lrelease.pro -o Makefile
endef

define $(package)_build_cmds
  $(MAKE) -C src $(addprefix sub-,$($(package)_qt_libs)) && \
  $(MAKE) -C ../qttools/src/linguist/lrelease && \
  $(MAKE) -C ../qttranslations
endef

define $(package)_stage_cmds
  $(MAKE) -C src INSTALL_ROOT=$($(package)_staging_dir) $(addsuffix -install_subtargets,$(addprefix sub-,$($(package)_qt_libs))) && cd .. &&\
  $(MAKE) -C qttools/src/linguist/lrelease INSTALL_ROOT=$($(package)_staging_dir) install_target && \
  $(MAKE) -C qttranslations INSTALL_ROOT=$($(package)_staging_dir) install_subtargets
endef

define $(package)_postprocess_cmds
  rm -rf native/mkspecs/ native/lib/ lib/cmake/ && \
  rm -f lib/lib*.la lib/*.prl plugins/*/*.prl
endef
