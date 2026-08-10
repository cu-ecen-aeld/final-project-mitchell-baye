#!/bin/sh
#
# wifi-connect: add a WiFi network to the running wpa_supplicant instance
# and persist it, without needing to rebuild/reflash the Yocto image.
#
# Usage: wifi-connect <ssid> <psk> [interface]
#
# Example:
#   wifi-connect "My Network" "mypassword123"
#   wifi-connect "Office WiFi" "officepass" wlan0

set -e

SSID="$1"
PSK="$2"
IFACE="${3:-wlan0}"

if [ -z "$SSID" ] || [ -z "$PSK" ]; then
    echo "Usage: $0 <ssid> <psk> [interface]" >&2
    exit 1
fi

echo "Adding network '$SSID' on $IFACE..."

NET_ID=$(wpa_cli -i "$IFACE" add_network | tail -n1)

if [ -z "$NET_ID" ] || ! echo "$NET_ID" | grep -qE '^[0-9]+$'; then
    echo "Failed to add network (is wpa_supplicant running on $IFACE?)" >&2
    exit 1
fi

wpa_cli -i "$IFACE" set_network "$NET_ID" ssid "\"$SSID\"" > /dev/null
wpa_cli -i "$IFACE" set_network "$NET_ID" psk "\"$PSK\"" > /dev/null
wpa_cli -i "$IFACE" enable_network "$NET_ID" > /dev/null
wpa_cli -i "$IFACE" select_network "$NET_ID" > /dev/null
wpa_cli -i "$IFACE" save_config > /dev/null

echo "Network '$SSID' added, selected, and saved."
echo "Waiting for association..."

sleep 5
STATUS=$(wpa_cli -i "$IFACE" status | grep '^wpa_state=')
echo "$STATUS"

if echo "$STATUS" | grep -q "COMPLETED"; then
    echo "Connected successfully."
    ip -4 addr show "$IFACE" | grep inet
else
    echo "Not yet connected (state above). Check 'journalctl -u wpa_supplicant@${IFACE}' if this persists." >&2
fi
