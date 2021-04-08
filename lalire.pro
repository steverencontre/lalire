QT       += core gui widgets

CONFIG += c++2a

xxQMAKE_CXXFLAGS += \
    -fmodules \
    -fbuiltin-module-map

DEFINES += QT_DISABLE_DEPRECATED_BEFORE=0x060000    # disables all the APIs deprecated before Qt 6.0.0

SOURCES += \
    Sipper.cpp \
    main.cpp \
    Window.cpp

HEADERS += \
    Sipper.h \
    Window.h

LIBS += \
    -lssl \
    -lcrypto \
    -lportaudio \
    -lasound \
    -luuid \
	\
	`PKG_CONFIG_PATH=/usr/local/lib/pkgconfig pkgconf --libs libpjproject`	\
	\
	-lspeex-x86_64-unknown-linux-gnu \
	-lresample-x86_64-unknown-linux-gnu \
	-lsrtp-x86_64-unknown-linux-gnu \
	-lwebrtc-x86_64-unknown-linux-gnu \
	-lyuv-x86_64-unknown-linux-gnu \
	-lg7221codec-x86_64-unknown-linux-gnu \
	-lgsmcodec-x86_64-unknown-linux-gnu \
	-lilbccodec-x86_64-unknown-linux-gnu
