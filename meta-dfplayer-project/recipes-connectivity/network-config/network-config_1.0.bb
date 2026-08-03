# network-config_1.0.bb
SUMMARY = "systemd-networkd configuration for wlan0"
LICENSE = "CLOSED"

SRC_URI = "file://10-wlan-dhcp.network"

S = "${WORKDIR}"

do_install() {
    install -d ${D}${sysconfdir}/systemd/network
    install -m 0644 ${WORKDIR}/10-wlan-dhcp.network ${D}${sysconfdir}/systemd/network/
}

FILES:${PN} += "${sysconfdir}/systemd/network/10-wlan-dhcp.network"
