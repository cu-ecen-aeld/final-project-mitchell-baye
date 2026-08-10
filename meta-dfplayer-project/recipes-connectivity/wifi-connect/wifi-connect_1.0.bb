SUMMARY = "Interactive WiFi network add/switch helper"
DESCRIPTION = "Wraps wpa_cli to add and persist new WiFi networks on a \
running wpa_supplicant instance, without needing to rebuild or reflash \
the image to switch networks"
LICENSE = "MIT"
LIC_FILES_CHKSUM = "file://${COMMON_LICENSE_DIR}/MIT;md5=0835ade698e0bcf8506ecda2f7b4f302"

SRC_URI = "file://wifi-connect.sh"

S = "${WORKDIR}"

do_install() {
    install -d ${D}${bindir}
    install -m 0755 wifi-connect.sh ${D}${bindir}/wifi-connect
}

RDEPENDS:${PN} = "wpa-supplicant"

FILES:${PN} = "${bindir}/wifi-connect"
