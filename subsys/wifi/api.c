/*
 * @file
 * @brief APIs for the external caller
 */

/*
 * Copyright (c) 2018, Unisoc Communications Inc
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#define LOG_LEVEL CONFIG_WIFIMGR_LOG_LEVEL
#include <logging/log.h>
LOG_MODULE_DECLARE(wifimgr);

#include "wifimgr.h"

static struct wifimgr_ctrl_cbs *wifimgr_cbs = NULL;

static int wifimgr_ctrl_iface_send_cmd(unsigned int cmd_id, void *buf,
				       int buf_len)
{
	struct mq_des *mq;
	struct mq_attr attr;
	struct cmd_message msg;
	struct timespec ts;
	int prio;
	int ret;

	attr.mq_maxmsg = WIFIMGR_CMD_MQUEUE_NR;
	attr.mq_msgsize = sizeof(msg);
	attr.mq_flags = 0;
	mq = mq_open(WIFIMGR_CMD_MQUEUE, O_RDWR | O_CREAT, 0666, &attr);
	if (!mq) {
		wifimgr_err("failed to open command queue %s!\n",
			    WIFIMGR_CMD_MQUEUE);
		return -errno;
	}

	msg.cmd_id = cmd_id;
	msg.reply = 0;
	msg.buf_len = buf_len;
	msg.buf = NULL;
	if (buf_len) {
		msg.buf = malloc(buf_len);
		memcpy(msg.buf, buf, buf_len);
	}

	ret = mq_send(mq, (const char *)&msg, sizeof(msg), 0);
	if (ret < 0) {
		wifimgr_err("failed to send [%s]: %d, errno %d!\n",
			    wifimgr_cmd2str(msg.cmd_id), ret, errno);
		ret = -errno;
	} else {
		wifimgr_dbg("send [%s], buf: 0x%08x\n",
			    wifimgr_cmd2str(msg.cmd_id), *(int *)msg.buf);

		ret = clock_gettime(CLOCK_MONOTONIC, &ts);
		if (ret)
			wifimgr_err("failed to get clock time: %d!\n", ret);
		ts.tv_sec += WIFIMGR_CMD_TIMEOUT;
		ret =
		    mq_timedreceive(mq, (char *)&msg, sizeof(msg), &prio, &ts);
		if (ret != sizeof(struct cmd_message)) {
			wifimgr_err
			    ("failed to get command reply: %d, errno %d!\n",
			     ret, errno);
			if (errno == ETIME)
				wifimgr_err("[%s] timeout!\n",
					    wifimgr_cmd2str(msg.cmd_id));
			ret = -errno;
		} else {
			wifimgr_dbg("recv [%s] reply: %d\n",
				    wifimgr_cmd2str(msg.cmd_id), msg.reply);
			ret = msg.reply;
			if (ret)
				wifimgr_err("failed to exec [%s]: %d!\n",
					    wifimgr_cmd2str(msg.cmd_id), ret);
		}
	}

	free(msg.buf);
	mq_close(mq);

	return ret;
}

int wifimgr_ctrl_iface_set_conf(char *iface_name, char *ssid, char *bssid,
				char *passphrase, unsigned char band,
				unsigned char channel,
				unsigned char channel_width)
{
	struct wifimgr_config conf;
	unsigned int cmd_id = 0;

	if (!iface_name)
		return -EINVAL;
	if ((strlen(ssid) > sizeof(conf.ssid))
	    || (strlen(passphrase) > sizeof(conf.passphrase)))
		return -EINVAL;

	memset(&conf, 0, sizeof(conf));
	if (ssid && strlen(ssid))
		strcpy(conf.ssid, ssid);
	if (bssid && !is_zero_ether_addr(bssid))
		memcpy(conf.bssid, bssid, WIFIMGR_ETH_ALEN);
	if (passphrase && strlen(passphrase))
		strcpy(conf.passphrase, passphrase);
	if (band)
		conf.band = band;
	if (channel)
		conf.channel = channel;
	if (channel_width)
		conf.channel_width = channel_width;

	if (!strcmp(iface_name, WIFIMGR_IFACE_NAME_STA))
		cmd_id = WIFIMGR_CMD_SET_STA_CONFIG;
	else if (!strcmp(iface_name, WIFIMGR_IFACE_NAME_AP))
		cmd_id = WIFIMGR_CMD_SET_AP_CONFIG;
	else
		return -EINVAL;

	return wifimgr_ctrl_iface_send_cmd(cmd_id, &conf, sizeof(conf));
}

int wifimgr_ctrl_iface_get_conf(char *iface_name)
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

int wifimgr_ctrl_iface_get_status(char *iface_name)
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

int wifimgr_ctrl_iface_open(char *iface_name)
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

int wifimgr_ctrl_iface_close(char *iface_name)
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

int wifimgr_ctrl_iface_scan(void)
{
	return wifimgr_ctrl_iface_send_cmd(WIFIMGR_CMD_SCAN, NULL, 0);
}

int wifimgr_ctrl_iface_connect(void)
{
	return wifimgr_ctrl_iface_send_cmd(WIFIMGR_CMD_CONNECT, NULL, 0);
}

int wifimgr_ctrl_iface_disconnect(void)
{
	return wifimgr_ctrl_iface_send_cmd(WIFIMGR_CMD_DISCONNECT, NULL, 0);
}

int wifimgr_ctrl_iface_start_ap(void)
{
	return wifimgr_ctrl_iface_send_cmd(WIFIMGR_CMD_START_AP, NULL, 0);
}

int wifimgr_ctrl_iface_stop_ap(void)
{
	return wifimgr_ctrl_iface_send_cmd(WIFIMGR_CMD_STOP_AP, NULL, 0);
}

int wifimgr_ctrl_iface_del_station(char *mac)
{
	struct wifimgr_del_station del_sta;

	if (mac && !is_zero_ether_addr(mac))
		memcpy(del_sta.mac, mac, WIFIMGR_ETH_ALEN);
	else
		memset(del_sta.mac, 0xff, WIFIMGR_ETH_ALEN);

	return wifimgr_ctrl_iface_send_cmd(WIFIMGR_CMD_DEL_STATION, &del_sta,
					   sizeof(del_sta));
}

static const struct wifimgr_ctrl_ops wifimgr_ops = {
	.set_conf = wifimgr_ctrl_iface_set_conf,
	.get_conf = wifimgr_ctrl_iface_get_conf,
	.get_status = wifimgr_ctrl_iface_get_status,
	.open = wifimgr_ctrl_iface_open,
	.close = wifimgr_ctrl_iface_close,
	.scan = wifimgr_ctrl_iface_scan,
	.connect = wifimgr_ctrl_iface_connect,
	.disconnect = wifimgr_ctrl_iface_disconnect,
	.start_ap = wifimgr_ctrl_iface_start_ap,
	.stop_ap = wifimgr_ctrl_iface_stop_ap,
	.del_station = wifimgr_ctrl_iface_del_station,
};

const
struct wifimgr_ctrl_ops *wifimgr_get_ctrl_ops(struct wifimgr_ctrl_cbs *cbs)
{
	wifimgr_cbs = cbs;

	return &wifimgr_ops;
}

struct wifimgr_ctrl_cbs *wifimgr_get_ctrl_cbs(void)
{
	return wifimgr_cbs;
}