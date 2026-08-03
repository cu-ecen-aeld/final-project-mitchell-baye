#wpa-supplicant_%.bbappend
SYSTEMD_SERVICE:${PN} = "wpa_supplicant@wlan0.service"
SYSTEMD_AUTO_ENABLE:pn-wpa-supplicant = "enable"
