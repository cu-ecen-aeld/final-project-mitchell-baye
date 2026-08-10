SUMMARY = "DFPlayer Mini control daemon"
DESCRIPTION = "Persistent daemon that owns the DFPlayer UART connection and \
accepts text commands over a unix domain socket from other daemons (IR \
remote, RF remote, etc.)"
LICENSE = "MIT"
LIC_FILES_CHKSUM = "file://${COMMON_LICENSE_DIR}/MIT;md5=0835ade698e0bcf8506ecda2f7b4f302"

SRC_URI = "file://dfplayer_daemon.c \
           file://dfplayer_lib.c \
           file://dfplayer_ctl.h \
           file://dfplayer-daemon.service \
"

S = "${WORKDIR}"

do_compile() {
    ${CC} ${CFLAGS} -c dfplayer_lib.c -o dfplayer_lib.o
    ${CC} ${CFLAGS} ${LDFLAGS} -o dfplayer-daemon dfplayer_daemon.c dfplayer_lib.o
}

do_install() {
    install -d ${D}${bindir}
    install -m 0755 dfplayer-daemon ${D}${bindir}

    install -d ${D}${systemd_system_unitdir}
    install -m 0644 ${WORKDIR}/dfplayer-daemon.service ${D}${systemd_system_unitdir}
}

inherit systemd
SYSTEMD_SERVICE:${PN} = "dfplayer-daemon.service"
SYSTEMD_AUTO_ENABLE = "enable"

FILES:${PN} += "${systemd_system_unitdir}/dfplayer-daemon.service"
