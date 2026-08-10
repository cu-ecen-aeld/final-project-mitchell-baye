SUMMARY = "IR remote scancode capture and logging tool"
DESCRIPTION = "Captures distinct IR scancodes from the rc-core input event \
interface and logs each new one to a CSV file for later reference when \
building a scancode-to-command mapping. Meant to be run interactively, \
not as a persistent service."
LICENSE = "MIT"
LIC_FILES_CHKSUM = "file://${COMMON_LICENSE_DIR}/MIT;md5=0835ade698e0bcf8506ecda2f7b4f302"

SRC_URI = "file://ir_scancode_logger.c"

S = "${WORKDIR}"

do_compile() {
    ${CC} ${CFLAGS} ${LDFLAGS} -o ir-scancode-logger ir_scancode_logger.c
}

do_install() {
    install -d ${D}${bindir}
    install -m 0755 ir-scancode-logger ${D}${bindir}
}
