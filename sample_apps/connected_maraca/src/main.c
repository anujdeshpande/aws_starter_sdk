/*
 *  Copyright (C) 2008-2016, Marvell International Ltd.
 *  All Rights Reserved.
 */
/*
 * Connected Maraca Application
 *
 * Summary:
 *
 * Device publishes the changed state of the maraca on
 * Thing Shadow.
 * The serial console is set on UART-0.
 *
 * A serial terminal program like HyperTerminal, putty, or
 * minicom can be used to see the program output.
 */

#include <wm_os.h>
#include <wmstdio.h>
#include <wmtime.h>
#include <wmsdk.h>
#include <led_indicator.h>
#include <board.h>
#include <aws_iot_mqtt_interface.h>
#include <aws_iot_shadow_interface.h>
#include <aws_utils.h>
#include <mdev_gpio.h>
#include <mdev_pinmux.h>
#include <lowlevel_drivers.h>
#include <wm_os.h>
#include <mdev_i2c.h>
#include <push_button.h>

/* configuration parameters */
#include <aws_iot_config.h>

#include "aws_starter_root_ca_cert.h"


#include "sensor_acc_drv.h"
#include "sensor_acc_drv.c"
#include "sensor_drv.h"
#include "sensor_drv.c"

enum state {
	AWS_CONNECTED = 1,
	AWS_RECONNECTED,
	AWS_DISCONNECTED
};

/*-----------------------Global declarations----------------------*/

/* These hold each pushbutton's count, updated in the callback ISR */
static volatile uint32_t led_1_state;
static volatile uint32_t led_1_state_prev = -1;

static output_gpio_cfg_t led_1;
static MQTTClient_t mqtt_client;
static enum state device_state;

/* Thread handle */
static os_thread_t aws_starter_thread;
/* Buffer to be used as stack */
static os_thread_stack_define(aws_starter_stack, 10 * 1024);
/* Thread handle */
static os_thread_t aws_shadow_yield_thread;
/* Buffer to be used as stack */
static os_thread_stack_define(aws_shadow_yield_stack, 2 * 1024);
/* aws iot url */
static char url[128];

#define MICRO_AP_SSID                "aws_starter"
#define MICRO_AP_PASSPHRASE          "marvellwm"
#define AMAZON_ACTION_BUF_SIZE  100
#define VAR_LED_1_PROPERTY      "led"
#define RESET_TO_FACTORY_TIMEOUT 5000
#define BUFSIZE                  200
#define THRESHOLD_ACC            50

/* callback function invoked on reset to factory */
static void device_reset_to_factory_cb()
{
	/* Clears device configuration settings from persistent memory
	 * and reboots the device.
	 */
	invoke_reset_to_factory();
}

/* board_button_2() is configured to perform reset to factory,
 * when pressed for more than 5 seconds.
 */
static void configure_reset_to_factory()
{
	input_gpio_cfg_t pushbutton_reset_to_factory = {
		.gpio = board_button_2(),
		.type = GPIO_ACTIVE_LOW
	};
	push_button_set_cb(pushbutton_reset_to_factory,
			   device_reset_to_factory_cb,
			   RESET_TO_FACTORY_TIMEOUT, 0, NULL);
}


/* Configure led and pushbuttons with callback functions */
static void configure_led_and_button()
{

	led_1 = board_led_1();

}

static char client_cert_buffer[AWS_PUB_CERT_SIZE];
static char private_key_buffer[AWS_PRIV_KEY_SIZE];
#define THING_LEN 126
#define REGION_LEN 16
static char thing_name[THING_LEN];

/* populate aws shadow configuration details */
static int aws_starter_load_configuration(ShadowParameters_t *sp)
{
	int ret = WM_SUCCESS;
	char region[REGION_LEN];
	memset(region, 0, sizeof(region));

	/* read configured thing name from the persistent memory */
	ret = read_aws_thing(thing_name, THING_LEN);
	if (ret == WM_SUCCESS) {
		sp->pMyThingName = thing_name;
	} else {
		/* if not found in memory, take the default thing name */
		sp->pMyThingName = AWS_IOT_MY_THING_NAME;
	}
	sp->pMqttClientId = AWS_IOT_MQTT_CLIENT_ID;

	/* read configured region name from the persistent memory */
	ret = read_aws_region(region, REGION_LEN);
	if (ret == WM_SUCCESS) {
		snprintf(url, sizeof(url), "data.iot.%s.amazonaws.com",
			 region);
	} else {
		snprintf(url, sizeof(url), "data.iot.%s.amazonaws.com",
			 AWS_IOT_MY_REGION_NAME);
	}
	sp->pHost = url;
	sp->port = AWS_IOT_MQTT_PORT;
	sp->pRootCA = rootCA;

	/* read configured certificate from the persistent memory */
	ret = read_aws_certificate(client_cert_buffer, AWS_PUB_CERT_SIZE);
	if (ret != WM_SUCCESS) {
		wmprintf("Failed to configure certificate. Returning!\r\n");
		return -WM_FAIL;
	}
	sp->pClientCRT = client_cert_buffer;

	/* read configured private key from the persistent memory */
	ret = read_aws_key(private_key_buffer, AWS_PRIV_KEY_SIZE);
	if (ret != WM_SUCCESS) {
		wmprintf("Failed to configure key. Returning!\r\n");
		return -WM_FAIL;
	}
	sp->pClientKey = private_key_buffer;

	return ret;
}

void shadow_update_status_cb(const char *pThingName, ShadowActions_t action,
			     Shadow_Ack_Status_t status,
			     const char *pReceivedJsonDocument,
			     void *pContextData) {

	if (status == SHADOW_ACK_TIMEOUT) {
		wmprintf("Shadow publish state change timeout occurred\r\n");
	} else if (status == SHADOW_ACK_REJECTED) {
		wmprintf("Shadow publish state change rejected\r\n");
	} else if (status == SHADOW_ACK_ACCEPTED) {
		wmprintf("Shadow publish state change accepted\r\n");
	}
}

/* shadow yield thread which periodically checks for data */
static void aws_shadow_yield(os_thread_arg_t data)
{
	while (1) {
		/* periodically check if any data is received on socket */
		aws_iot_shadow_yield(&mqtt_client, 500);
	}
}

/* create shadow yield thread */
static int aws_create_shadow_yield_thread()
{
	int ret;
	ret = os_thread_create(
		/* thread handle */
		&aws_shadow_yield_thread,
		/* thread name */
		"awsShadowYield",
		/* entry function */
		aws_shadow_yield,
		/* argument */
		0,
		/* stack */
		&aws_shadow_yield_stack,
		/* priority */
		OS_PRIO_3);
	if (ret != WM_SUCCESS) {
		wmprintf("Failed to create shadow yield thread: %d\r\n", ret);
		return -WM_FAIL;
	}
	return WM_SUCCESS;
}

/* This function will get invoked when led state change request is received */
void led_indicator_cb(const char *p_json_string,
		      uint32_t json_string_datalen,
		      jsonStruct_t *p_context) {
	int state;
	if (p_context != NULL) {
		state = *(int *)(p_context->pData);
		if (state) {
			led_on(led_1);
			led_1_state = 1;
	} else {
			led_off(led_1);
			led_1_state = 0;
		}
	}
}

/* Publish thing state to shadow */
int aws_publish_property_state(ShadowParameters_t *sp)
{
	char buf_out[BUFSIZE];
	char state[BUFSIZE];
	char *ptr = state;
	int ret = WM_SUCCESS;

	memset(state, 0, BUFSIZE);
	
	/* Get sensor readings, compare with previous values, if delta is greater
	   than THRESHOLD_ACC, publish the state
	*/
	/* Construct JSON object for sensor events */
	sensor_msg_construct(state, buf_out, BUFSIZE);

	if (*ptr == ',')
		ptr++;
	 if (strlen(state)) { 

	snprintf(buf_out, BUFSIZE, "{\"state\":  {\"reported\":{\"device_id\": \"mrvlmw302\",\"time\":\"\",\"device\":\"marvelliot\",\"sensors\":[{\"telemetryData\": {%s}}]}}}", ptr);

	MQTTPublishParams cmaraca;
	
	memset(&cmaraca, 0, sizeof(cmaraca));   
	cmaraca.pTopic = "connected-maraca";
	cmaraca.MessageParams.pPayload = buf_out;
	cmaraca.MessageParams.PayloadLen = strlen(buf_out);
	aws_iot_mqtt_publish(&cmaraca);

	wmprintf("\r\n%s", buf_out);
	
	/* publish incremented value on pushbutton press on
	 * configured thing */
	/* ret = aws_iot_shadow_update(&mqtt_client, */
	/* 			    sp->pMyThingName, */
	/* 			    buf_out, */
	/* 			    shadow_update_status_cb, */
	/* 			    NULL, */
	/* 			    10, true); */
	
	/*os_thread_sleep(10000);*/
	 } 
	return ret;
}

/* application thread */
static void connected_maraca(os_thread_arg_t data)
{
        int led_state = 0;
        int ret;
	jsonStruct_t led_indicator;
	ShadowParameters_t sp;

	aws_iot_mqtt_init(&mqtt_client);

	ret = aws_starter_load_configuration(&sp);
	if (ret != WM_SUCCESS) {
		wmprintf("aws shadow configuration failed : %d\r\n", ret);
		goto out;
	}

	ret = aws_iot_shadow_init(&mqtt_client);
	if (ret != WM_SUCCESS) {
		wmprintf("aws shadow init failed : %d\r\n", ret);
		goto out;
	}

	ret = aws_iot_shadow_connect(&mqtt_client, &sp);
	if (ret != WM_SUCCESS) {
		wmprintf("aws shadow connect failed : %d\r\n", ret);
		goto out;
	}

	/* indication that device is connected and cloud is started */
	led_on(board_led_2());
	wmprintf("Cloud Started\r\n");

	/* configures property of a thing */
	led_indicator.cb = led_indicator_cb;
	led_indicator.pData = &led_state;
	led_indicator.pKey = "led";
	led_indicator.type = SHADOW_JSON_INT8;

	/* subscribes to delta topic of the configured thing */
	ret = aws_iot_shadow_register_delta(&mqtt_client, &led_indicator);
	if (ret != WM_SUCCESS) {
		wmprintf("Failed to subscribe to shadow delta %d\r\n", ret);
		goto out;
	}

	/* creates a thread which will wait for incoming messages, ensuring the
	 * connection is kept alive with the AWS Service
	 */
	aws_create_shadow_yield_thread();

	while (1) {
		/* Implement application logic here */

		if (device_state == AWS_RECONNECTED) {
			ret = aws_iot_shadow_connect(&mqtt_client, &sp);
			if (ret != WM_SUCCESS) {
				wmprintf("aws shadow reconnect failed: "
					 "%d\r\n", ret);
				goto out;
			} else {
				device_state = AWS_CONNECTED;
				led_on(board_led_2());
			}
		}

		ret = aws_publish_property_state(&sp);
		if (ret != WM_SUCCESS)
			wmprintf("Sending property failed\r\n");

		os_thread_sleep(1000);
		sensor_inputs_scan();
	}
	
	ret = aws_iot_shadow_disconnect(&mqtt_client);
	if (NONE_ERROR != ret) {
		wmprintf("aws iot shadow error %d\r\n", ret);
	}

out:
	os_thread_self_complete(NULL);
	return;
}

void wlan_event_normal_link_lost(void *data)
{
	/* led indication to indicate link loss */
	aws_iot_shadow_disconnect(&mqtt_client);
	device_state = AWS_DISCONNECTED;
}

void wlan_event_normal_connect_failed(void *data)
{
	/* led indication to indicate connect failed */
	aws_iot_shadow_disconnect(&mqtt_client);
	device_state = AWS_DISCONNECTED;
}

/* This function gets invoked when station interface connects to home AP.
 * Network dependent services can be started here.
 */
void wlan_event_normal_connected(void *data)
{
	int ret;
	/* Default time set to 1 October 2015 */
	time_t time = 1443657600;

	wmprintf("Connected successfully to the configured network\r\n");

	if (!device_state) {
		/* set system time */
		wmtime_time_set_posix(time);

		/* create cloud thread */
		ret = os_thread_create(
			/* thread handle */
			&aws_starter_thread,
			/* thread name */
			"connectedMaraca",
			/* entry function */
			connected_maraca,
			/* argument */
			0,
			/* stack */
			&aws_starter_stack,
			/* priority */
			OS_PRIO_3);
		if (ret != WM_SUCCESS) {
			wmprintf("Failed to start cloud_thread: %d\r\n", ret);
			return;
		}
	}

	if (!device_state)
		device_state = AWS_CONNECTED;
	else if (device_state == AWS_DISCONNECTED)
		device_state = AWS_RECONNECTED;
}

int main()
{
	/* initialize the standard input output facility over uart */
	if (wmstdio_init(UART0_ID, 0) != WM_SUCCESS) {
		return -WM_FAIL;
	}

	wmprintf("Build Time: " __DATE__ " " __TIME__ "\r\n");
	wmprintf("\r\n#### CONNECTED MARACA DEMO ####\r\n\r\n");

	/* initialize gpio driver */
	if (gpio_drv_init() != WM_SUCCESS) {
		wmprintf("gpio_drv_init failed\r\n");
		return -WM_FAIL;
	}
	int retval = sensor_drv_init();
	if (retval == WM_SUCCESS) {
	  acc_sensor_event_register();
	}

	/* configure pushbutton on device to perform reset to factory */
	configure_reset_to_factory();
	/* configure led and pushbutton to communicate with cloud */
	configure_led_and_button();

	/* This api adds aws iot configuration support in web application.
	 * Configuration details are then stored in persistent memory.
	 */
	enable_aws_config_support();

	/* This api starts micro-AP if device is not configured, else connects
	 * to configured network stored in persistent memory. Function
	 * wlan_event_normal_connected() is invoked on successful connection.
	 */
	wm_wlan_start(MICRO_AP_SSID, MICRO_AP_PASSPHRASE);
	return 0;
}
