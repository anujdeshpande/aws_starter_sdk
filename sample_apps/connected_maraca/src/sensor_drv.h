/*
 *  Copyright (C) 2015-2016, Marvell International Ltd.
 *  All Rights Reserved.
 */

/*
 * Sensor Event Driver header file
 */

#ifndef __SENSOR_DRV_H__
#define __SENSOR_DRV_H__

/* Time interval with which each sensor event will be scanned
	in the periodic fashion
*/
#define SENSOR_POLL_TIME	20 /* (in Miliseconds) */

#if SENSOR_DEBUG_ENABLE
#define dbg(_fmt_, ...)				\
	wmprintf("[appln] "_fmt_"\n\r", ##__VA_ARGS__)
#else
#define dbg(...)
#endif /* SENSOR_DEBUG_ENABLE */


/*-----------------------Global declarations----------------------*/

/* Struct to hold each sensor event data */
struct sensor_info {
	char event_curr_value[64];
	char event_prev_value[64];
	char *property; /*JSON Name of event */
	int (*init)(struct sensor_info *sevent);
	int (*write)(struct sensor_info *sevent);
	int (*read)(struct sensor_info *sevent);
	int (*exit)(struct sensor_info *sevent);
	struct sensor_info *next;
};

int sensor_event_register(struct sensor_info *sevnt);
int sensor_drv_init(void);
int sensor_msg_construct(char *src, char*dest, int len);
void sensor_inputs_scan(void);

#endif /* __SENSOR_DRV_H__ */
