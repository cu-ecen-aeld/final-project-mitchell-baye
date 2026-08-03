SUMMARY = "wpa_supplicant configuration for wlan0"
LICENSE = "CLOSED"

SRC_URI = "file://wpa_supplicant-wlan0.conf"

S = "${WORKDIR}"

do_install() {
    install -d ${D}${sysconfdir}/wpa_supplicant
    install -m 0600 ${WORKDIR}/wpa_supplicant-wlan0.conf ${D}${sysconfdir}/wpa_supplicant/
}

FILES:${PN} += "${sysconfdir}/wpa_supplicant/wpa_supplicant-wlan0.conf"
