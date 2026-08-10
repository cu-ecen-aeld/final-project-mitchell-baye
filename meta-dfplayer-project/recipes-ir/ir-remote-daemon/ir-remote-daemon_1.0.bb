SUMMARY = "IR remote scancode-to-DFPlayer-command daemon"
DESCRIPTION = "Reads decoded IR scancodes from the rc-core input device and \
forwards the mapped DFPlayer command to dfplayer-daemon over a unix domain \
socket"
LICENSE = "MIT"
LIC_FILES_CHKSUM = "file://${COMMON_LICENSE_DIR}/MIT;md5=0835ade698e0bcf8506ecda2f7b4f302"

SRC_URI = "file://ir_remote_daemon.c \
           file://ir-remote-daemon.service \
"

S = "${WORKDIR}"

do_compile() {
    ${CC} ${CFLAGS} ${LDFLAGS} -o ir-remote-daemon ir_remote_daemon.c
}

do_install() {
    install -d ${D}${bindir}
    install -m 0755 ir-remote-daemon ${D}${bindir}

    install -d ${D}${systemd_system_unitdir}
    install -m 0644 ${WORKDIR}/ir-remote-daemon.service ${D}${systemd_system_unitdir}
}

inherit systemd
SYSTEMD_SERVICE:${PN} = "ir-remote-daemon.service"
SYSTEMD_AUTO_ENABLE = "enable"

FILES:${PN} += "${systemd_system_unitdir}/ir-remote-daemon.service"
