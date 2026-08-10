SUMMARY = "IR Remote Proof of Concept"
DESCRIPTION = "Proof of concept tool for reading and differentiating IR \
remote scancodes via the rc-core input event interface. Meant to be run \
interactively, not as a persistent service."
LICENSE = "MIT"
LIC_FILES_CHKSUM = "file://${COMMON_LICENSE_DIR}/MIT;md5=0835ade698e0bcf8506ecda2f7b4f302"

SRC_URI = "file://ir_remote_poc.c"

S = "${WORKDIR}"

do_compile() {
    ${CC} ${CFLAGS} ${LDFLAGS} -o ir-remote-poc ir_remote_poc.c
}

do_install() {
    install -d ${D}${bindir}
    install -m 0755 ir-remote-poc ${D}${bindir}
}
