/*
 * @file
 * @brief APIs for the external caller
 */

/*
 * Copyright (c) 2018, Unisoc Communications Inc
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#if defined(CONFIG_WIFIMGR_STA) || defined(CONFIG_WIFIMGR_AP)

#include <net/wifimgr_api.h>

#include "api.h"
#include "config.h"
#include "timer.h"

extern timer_t sta_autorun_timerid;
extern timer_t ap_autorun_timerid;

static struct wifimgr_ctrl_cbs *wifimgr_cbs;

int wifimgr_get_ctrl(char *iface_name)
{
	unsigned int cmd_id = 0;

	if (!iface_name)
		return -EINVAL;

	if (!strcmp(iface_name, WIFIMGR_IFACE_NAME_STA))
		cmd_id = WIFIMGR_CMD_GET_STA_CTRL;
	else if (!strcmp(iface_name, WIFIMGR_IFACE_NAME_AP))
		cmd_id = WIFIMGR_CMD_GET_AP_CTRL;
	else
		return -EINVAL;

	return wifimgr_ctrl_iface_send_cmd(cmd_id, NULL, 0);
}

int wifimgr_release_ctrl(char *iface_name)
{
	unsigned int cmd_id = 0;

	if (!iface_name)
		return -EINVAL;

	if (!strcmp(iface_name, WIFIMGR_IFACE_NAME_STA))
		cmd_id = WIFIMGR_CMD_RELEASE_STA_CTRL;
	else if (!strcmp(iface_name, WIFIMGR_IFACE_NAME_AP))
		cmd_id = WIFIMGR_CMD_RELEASE_AP_CTRL;
	else
		return -EINVAL;

	return wifimgr_ctrl_iface_send_cmd(cmd_id, NULL, 0);
}

static int wifimgr_ctrl_iface_get_conf(char *iface_name)
{
	unsigned int cmd_id = 0;

	if (!iface_name)
		return -EINVAL;

	if (!strcmp(iface_name, WIFIMGR_IFACE_NAME_STA))
		cmd_id = WIFIMGR_CMD_GET_STA_CONFIG;
	else if (!strcmp(iface_name, WIFIMGR_IFACE_NAME_AP))
		cmd_id = WIFIMGR_CMD_GET_AP_CONFIG;
	else
		return -EINVAL;

	return wifimgr_ctrl_iface_send_cmd(cmd_id, NULL, 0);
}

int wifimgr_ctrl_iface_set_conf(char *iface_name, char *ssid, char *bssid,
				char security, char *passphrase,
				unsigned char band, unsigned char channel,
				unsigned char ch_width, int autorun)
{
	struct wifimgr_config conf;
	timer_t autorun_timerid;
	unsigned int cmd_id;

	if (!iface_name)
		return -EINVAL;
	if (!strcmp(iface_name, WIFIMGR_IFACE_NAME_STA)) {
		cmd_id = WIFIMGR_CMD_SET_STA_CONFIG;
		autorun_timerid = sta_autorun_timerid;
	} else if (!strcmp(iface_name, WIFIMGR_IFACE_NAME_AP)) {
		cmd_id = WIFIMGR_CMD_SET_AP_CONFIG;
		autorun_timerid = ap_autorun_timerid;
	} else {
		return -EINVAL;
	}

	memset(&conf, 0, sizeof(conf));

	/* Check SSID (mandatory) */
	if (ssid) {
		if (!strlen(ssid) || (strlen(ssid) > sizeof(conf.ssid))) {
			printf("Invalid SSID: %s!\n", ssid);
			return -EINVAL;
		}
		strcpy(conf.ssid, ssid);
		printf("SSID:\t\t%s\n", ssid);
	}

	/* Check BSSID (optional) */
	if (bssid) {
		if (is_zero_ether_addr(bssid)) {
			printf("Invalid BSSID!\n");
			return -EINVAL;
		}
		memcpy(conf.bssid, bssid, WIFIMGR_ETH_ALEN);
		printf("BSSID:\t\t" MACSTR "\n", MAC2STR(bssid));
	}

	/* Check Security */
	switch (security) {
	case WIFIMGR_SECURITY_OPEN:
	case WIFIMGR_SECURITY_PSK:
		printf("Security:\t%s\n", security2str(security));
	case 0:
		conf.security = security;
		break;
	default:
		printf("invalid security: %d!\n", security);
		return -EINVAL;
	}

	/* Check Passphrase (optional: valid only for WPA/WPA2-PSK) */
	if (passphrase) {
		if (strlen(passphrase) > sizeof(conf.passphrase)) {
			printf("invalid PSK: %s!\n", passphrase);
			return -EINVAL;
		}
		strcpy(conf.passphrase, passphrase);
		if (strlen(passphrase))
			printf("Passphrase:\t%s\n", passphrase);
	}

	/* Check band */
	switch (band) {
	case 2:
	case 5:
		printf("Band:\t%u\n", band);
	case 0:
		conf.band = band;
		break;
	default:
		printf("invalid band: %u!\n", band);
		return -EINVAL;
	}

	/* Check channel */
	if ((channel > 14 && channel < 34) || (channel > 196)) {
		printf("invalid channel: %u!\n", channel);
		return -EINVAL;
	}
	conf.channel = channel;
	if (channel)
		printf("Channel:\t%u\n", channel);

	/* Check channel width */
	switch (ch_width) {
	case 20:
	case 40:
	case 80:
	case 160:
		printf("Channel Width:\t%u\n", ch_width);
	case 0:
		conf.ch_width = ch_width;
		break;
	default:
		printf("invalid channel width: %u!\n", ch_width);
		return -EINVAL;
	}

	/* Check autorun */
	conf.autorun = autorun;
	if (autorun)
		printf("----------------\n");
	if (autorun > 0) {
		printf("Autorun:\t%ds\n", autorun);
		wifimgr_timer_start(autorun_timerid, autorun);
	} else if (autorun < 0) {
		printf("Autorun:\toff\n");
		wifimgr_timer_stop(autorun_timerid);
	}

	return wifimgr_ctrl_iface_send_cmd(cmd_id, &conf, sizeof(conf));
}

static int wifimgr_ctrl_iface_clear_conf(char *iface_name)
{
	struct wifimgr_config conf;
	unsigned int cmd_id;

	memset(&conf, 0, sizeof(conf));

	if (!iface_name)
		return -EINVAL;

	if (!strcmp(iface_name, WIFIMGR_IFACE_NAME_STA))
		cmd_id = WIFIMGR_CMD_SET_STA_CONFIG;
	else if (!strcmp(iface_name, WIFIMGR_IFACE_NAME_AP))
		cmd_id = WIFIMGR_CMD_SET_AP_CONFIG;
	else
		return -EINVAL;

	return wifimgr_ctrl_iface_send_cmd(cmd_id, &conf, sizeof(conf));
}

static int wifimgr_ctrl_iface_get_capa(char *iface_name)
{
	unsigned int cmd_id = 0;

	if (!iface_name)
		return -EINVAL;

	if (!strcmp(iface_name, WIFIMGR_IFACE_NAME_STA))
		cmd_id = WIFIMGR_CMD_GET_STA_CAPA;
	else if (!strcmp(iface_name, WIFIMGR_IFACE_NAME_AP))
		cmd_id = WIFIMGR_CMD_GET_AP_CAPA;
	else
		return -EINVAL;

	return wifimgr_ctrl_iface_send_cmd(cmd_id, NULL, 0);
}

static int wifimgr_ctrl_iface_get_status(char *iface_name)
{
	unsigned int cmd_id = 0;

	if (!iface_name)
		return -EINVAL;

	if (!strcmp(iface_name, WIFIMGR_IFACE_NAME_STA))
		cmd_id = WIFIMGR_CMD_GET_STA_STATUS;
	else if (!strcmp(iface_name, WIFIMGR_IFACE_NAME_AP))
		cmd_id = WIFIMGR_CMD_GET_AP_STATUS;
	else
		return -EINVAL;

	return wifimgr_ctrl_iface_send_cmd(cmd_id, NULL, 0);
}

static int wifimgr_ctrl_iface_open(char *iface_name)
{
	unsigned int cmd_id = 0;

	if (!iface_name)
		return -EINVAL;

	if (!strcmp(iface_name, WIFIMGR_IFACE_NAME_STA))
		cmd_id = WIFIMGR_CMD_OPEN_STA;
	else if (!strcmp(iface_name, WIFIMGR_IFACE_NAME_AP))
		cmd_id = WIFIMGR_CMD_OPEN_AP;
	else
		return -EINVAL;

	return wifimgr_ctrl_iface_send_cmd(cmd_id, NULL, 0);
}

static int wifimgr_ctrl_iface_close(char *iface_name)
{
	unsigned int cmd_id = 0;

	if (!iface_name)
		return -EINVAL;

	if (!strcmp(iface_name, WIFIMGR_IFACE_NAME_STA))
		cmd_id = WIFIMGR_CMD_CLOSE_STA;
	else if (!strcmp(iface_name, WIFIMGR_IFACE_NAME_AP))
		cmd_id = WIFIMGR_CMD_CLOSE_AP;
	else
		return -EINVAL;

	return wifimgr_ctrl_iface_send_cmd(cmd_id, NULL, 0);
}

#ifdef CONFIG_WIFIMGR_STA
static int wifimgr_ctrl_iface_scan(void)
{
	return wifimgr_ctrl_iface_send_cmd(WIFIMGR_CMD_SCAN, NULL, 0);
}

static int wifimgr_ctrl_iface_connect(void)
{
	return wifimgr_ctrl_iface_send_cmd(WIFIMGR_CMD_CONNECT, NULL, 0);
}

static int wifimgr_ctrl_iface_disconnect(void)
{
	return wifimgr_ctrl_iface_send_cmd(WIFIMGR_CMD_DISCONNECT, NULL, 0);
}
#endif

#ifdef CONFIG_WIFIMGR_AP
static int wifimgr_ctrl_iface_start_ap(void)
{
	return wifimgr_ctrl_iface_send_cmd(WIFIMGR_CMD_START_AP, NULL, 0);
}

static int wifimgr_ctrl_iface_stop_ap(void)
{
	return wifimgr_ctrl_iface_send_cmd(WIFIMGR_CMD_STOP_AP, NULL, 0);
}

static int wifimgr_ctrl_iface_set_mac_acl(char subcmd, char *mac)
{
	struct wifimgr_set_mac_acl set_acl;

	switch (subcmd) {
	case WIFIMGR_SUBCMD_ACL_BLOCK:
	case WIFIMGR_SUBCMD_ACL_UNBLOCK:
	case WIFIMGR_SUBCMD_ACL_BLOCK_ALL:
	case WIFIMGR_SUBCMD_ACL_UNBLOCK_ALL:
		set_acl.subcmd = subcmd;
		break;
	default:
		return -EINVAL;
	}

	if (mac && !is_zero_ether_addr(mac)) {
		memcpy(set_acl.mac, mac, WIFIMGR_ETH_ALEN);
	} else if (!mac) {
		memset(set_acl.mac, 0xff, WIFIMGR_ETH_ALEN);
	} else {
		printf("invalid MAC address!\n");
		return -EINVAL;
	}

	return wifimgr_ctrl_iface_send_cmd(WIFIMGR_CMD_SET_MAC_ACL, &set_acl,
					   sizeof(set_acl));
}
#endif

static const struct wifimgr_ctrl_ops wifimgr_ops = {
	.get_conf = wifimgr_ctrl_iface_get_conf,
	.set_conf = wifimgr_ctrl_iface_set_conf,
	.clear_conf = wifimgr_ctrl_iface_clear_conf,
	.get_status = wifimgr_ctrl_iface_get_status,
	.get_capa = wifimgr_ctrl_iface_get_capa,
	.open = wifimgr_ctrl_iface_open,
	.close = wifimgr_ctrl_iface_close,
#ifdef CONFIG_WIFIMGR_STA
	.scan = wifimgr_ctrl_iface_scan,
	.connect = wifimgr_ctrl_iface_connect,
	.disconnect = wifimgr_ctrl_iface_disconnect,
#endif
#ifdef CONFIG_WIFIMGR_AP
	.start_ap = wifimgr_ctrl_iface_start_ap,
	.stop_ap = wifimgr_ctrl_iface_stop_ap,
	.set_mac_acl = wifimgr_ctrl_iface_set_mac_acl,
#endif
};

const struct wifimgr_ctrl_ops *wifimgr_get_ctrl_ops(void)
{
	return &wifimgr_ops;
}

const
struct wifimgr_ctrl_ops *wifimgr_get_ctrl_ops_cbs(struct wifimgr_ctrl_cbs *cbs)
{
	wifimgr_cbs = cbs;

	return &wifimgr_ops;
}

struct wifimgr_ctrl_cbs *wifimgr_get_ctrl_cbs(void)
{
	return wifimgr_cbs;
}

#endif