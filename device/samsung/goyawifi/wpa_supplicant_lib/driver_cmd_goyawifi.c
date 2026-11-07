/*
 * SPDX-License-Identifier: BSD-3-Clause
 *
 * wpa_supplicant private driver commands for the SM-T110 (Marvell SD8777, sd8xxx driver).
 *
 * Only COUNTRY is passed to the driver, through the Android private command ioctl, so
 * Android's Wi-Fi country code (Settings.Global.WIFI_COUNTRY_CODE) reaches the driver and
 * the kernel regulatory domain. Every other DRIVER command fails, as it did without a
 * private library, so the Wi-Fi behaviour tested so far doesn't change.
 */
#include "includes.h"
#include <sys/ioctl.h>
#include <net/if.h>

#include "common.h"
#include "driver_nl80211.h"
#include "wpa_supplicant_i.h"

typedef struct android_wifi_priv_cmd {
	char *buf;
	int used_len;
	int total_len;
} android_wifi_priv_cmd;

int wpa_driver_nl80211_driver_cmd(void *priv, char *cmd, char *buf, size_t buf_len)
{
	struct i802_bss *bss = priv;
	struct wpa_driver_nl80211_data *drv = bss->drv;
	android_wifi_priv_cmd priv_cmd;
	struct ifreq ifr;
	size_t len = os_strlen(cmd);

	if (os_strncasecmp(cmd, "COUNTRY ", 8) != 0 || len + 1 > buf_len)
		return -1;

	os_memcpy(buf, cmd, len + 1);
	os_memset(&ifr, 0, sizeof(ifr));
	os_memset(&priv_cmd, 0, sizeof(priv_cmd));
	os_strlcpy(ifr.ifr_name, bss->ifname, IFNAMSIZ);
	priv_cmd.buf = buf;
	priv_cmd.used_len = buf_len;
	priv_cmd.total_len = buf_len;
	ifr.ifr_data = (void *) &priv_cmd;

	if (ioctl(drv->global->ioctl_sock, SIOCDEVPRIVATE + 1, &ifr) < 0) {
		wpa_printf(MSG_ERROR, "%s: %s failed: %s", __func__, cmd, strerror(errno));
		return -1;
	}
	/* The kernel's regulatory change event updates wpa_supplicant's channel list. */
	wpa_printf(MSG_INFO, "%s: %s", __func__, cmd);
	buf[0] = '\0';
	return 0;
}

/* P2P hooks the Android build of driver_nl80211 expects from a private library. */
int wpa_driver_set_p2p_noa(void *priv, u8 count, int start, int duration)
{
	return 0;
}

int wpa_driver_get_p2p_noa(void *priv, u8 *buf, size_t len)
{
	return 0;
}

int wpa_driver_set_p2p_ps(void *priv, int legacy_ps, int opp_ps, int ctwindow)
{
	return -1;
}

int wpa_driver_set_ap_wps_p2p_ie(void *priv, const struct wpabuf *beacon,
				 const struct wpabuf *proberesp,
				 const struct wpabuf *assocresp)
{
	return 0;
}
