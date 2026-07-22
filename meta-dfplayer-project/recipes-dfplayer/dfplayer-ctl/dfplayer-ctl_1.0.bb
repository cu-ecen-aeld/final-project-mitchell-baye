SUMMARY = "DFPlayer Mini UART control utility"
DESCRIPTION = "Sends command frames to a DFPlayer Mini MP3 module over UART"
LICENSE = "MIT"
LIC_FILES_CHKSUM = "file://${COMMON_LICENSE_DIR}/MIT;md5=0835ade698e0bcf8506ecda2f7b4f302"

SRC_URI = "file://dfplayer_ctl.c \
            file://dfplayer_ctl.h \
            file://dfplayer-ctl.service \
            "

S = "${WORKDIR}"

do_compile() {
    ${CC} ${CFLAGS} ${LDFLAGS} -o dfplayer-ctl dfplayer_ctl.c
}

do_install() {
    install -d ${D}${bindir}
    install -m 0755 dfplayer-ctl ${D}${bindir}

    install -d ${D}${systemd_system_unitdir}
    install -m 0644 ${WORKDIR}/dfplayer-ctl.service ${D}${systemd_system_unitdir}
}

inherit systemd
SYSTEMD_SERVICE:${PN} = "dfplayer-ctl.service"
SYSTEMD_AUTO_ENABLE = "enable"

FILES:${PN} += "${systemd_system_unitdir}/dfplayer-ctl.service"
