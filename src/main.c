#include <zboss_api.h> // the Zigbee stack itself (ZBOSS) - joining network, sending packets, etc.
#include <zephyr/kernel.h>  // Zephyr RTOS - gives you k_sleep, threads, etc.
#include "zb_sensor_node.h"
#include <zboss_api_addons.h> // extra helper macros/functions on top of zboss_api.h
#include <zephyr/logging/log.h>
#include <zephyr/drivers/gpio.h>
#include <zigbee/zigbee_app_utils.h> // Nordic's helper functions (zigbee_enable, sleepy behavior, etc.)
#include <zigbee/zigbee_error_handler.h> // ZB_ERROR_CHECK macro

LOG_MODULE_REGISTER(btz, LOG_LEVEL_INF);

#define LIGHT_BULB_ENDPOINT 10
#define LED_NODE DT_ALIAS(led0)

static const struct gpio_dt_spec led = GPIO_DT_SPEC_GET(LED_NODE, gpios);

struct zb_device_ctx{
    zb_zcl_basic_attrs_ext_t basic_attr;
    zb_zcl_identify_attrs_t identify_attr;
    zb_zcl_on_off_attrs_t on_off_attr;
};

static struct zb_device_ctx dev_ctx;

static void app_clusters_attr_init(void){
    dev_ctx.basic_attr.zcl_version = ZB_ZCL_VERSION;
    dev_ctx.basic_attr.power_source =  ZB_ZCL_BASIC_POWER_SOURCE_BATTERY;
    dev_ctx.identify_attr.identify_time = ZB_ZCL_IDENTIFY_IDENTIFY_TIME_DEFAULT_VALUE;
    dev_ctx.on_off_attr.on_off = ZB_FALSE;
}

/* attribute lists*/
ZB_ZCL_DECLARE_BASIC_ATTRIB_LIST(basic_attrib_list, &dev_ctx.basic_attr.zcl_version, &dev_ctx.basic_attr.power_source);
ZB_ZCL_DECLARE_IDENTIFY_ATTRIB_LIST(identify_attr_list,&dev_ctx.identify_attr.identify_time);
ZB_ZCL_DECLARE_ON_OFF_ATTRIB_LIST(on_off_attr_list, &dev_ctx.on_off_attr.on_off);

/* cluster lists */
ZB_DECLARE_LIGHT_BULB_CLUSTER_LIST(light_bulb_clusters, basic_attrib_list, identify_attr_list, on_off_attr_list);

/* endpoint */
ZB_DECLARE_LIGHT_BULB_EP(light_bulb_ep, LIGHT_BULB_ENDPOINT, light_bulb_clusters);

/* device context */
ZBOSS_DECLARE_DEVICE_CTX_1_EP(light_bulb_ctx, light_bulb_ep);

static void on_off_set_value(zb_bool_t value){
    dev_ctx.on_off_attr.on_off = value;
    if(value){
        LOG_INF("LED IS ON!");
        gpio_pin_set_dt(&led, 1);
    }
    else{
        LOG_INF("LED IS OFF!");
        gpio_pin_set_dt(&led, 0);
    }
}

static zb_uint8_t zcl_device_cb(zb_bufid_t bufid){
    zb_zcl_device_callback_param_t *p = ZB_BUF_GET_PARAM(bufid, zb_zcl_device_callback_param_t); /* returns a pointer to the data in the buffer */
    p->status = RET_OK;

    if(p->device_cb_id != ZB_ZCL_SET_ATTR_VALUE_CB_ID){
        return ZB_FALSE;
    }

    zb_uint16_t cluster_id = p->cb_param.set_attr_value_param.cluster_id;
    zb_uint16_t attr_id = p->cb_param.set_attr_value_param.attr_id;

    if(cluster_id == ZB_ZCL_CLUSTER_ID_ON_OFF && attr_id == ZB_ZCL_ATTR_ON_OFF_ON_OFF_ID){
        zb_uint8_t value = p->cb_param.set_attr_value_param.values.data8;
        if(value){
            on_off_set_value(ZB_TRUE);
        }
        else{
            on_off_set_value(ZB_FALSE);
        }
    }
    return ZB_FALSE;
}

static void zboss_signal_handler(zb_bufid_t bufid){
    zb_zdo_app_signal_hdr_t *sg_p  = NULL;
    zb_zdo_app_signal_type_t  sig  = zb_get_app_signal(bufid, &sg_p);
    zb_ret_t status = ZB_GET_APP_SIGNAL_STATUS(bufid);

    switch(sig){
        case ZB_BDB_SIGNAL_DEVICE_FIRST_START:
            LOG_INF("Joining network for the first time...");
            bdb_start_top_level_commissioning(ZB_BDB_NETWORK_STEERING);
            break;

        case ZB_BDB_SIGNAL_DEVICE_REBOOT:
            bdb_start_top_level_commissioning(ZB_BDB_NETWORK_STEERING);
            break;

        case ZB_BDB_SIGNAL_STEERING:
            if(status == RET_OK){
                uint16_t panId = zb_get_pan_id();
                uint8_t channel = zb_get_current_channel();
                uint16_t shortAddr = zb_get_short_address();

                LOG_INF("Joined a network: PAN ID=0x%04X, Channel=%u, Short Addr=0x%04X", panId, channel, shortAddr);
            }
            else{
                /* ZB network unit of time is BI(beacon interval), ZB devices use it to schedule operations. Beacons because zb in built on ieee802.15.4 which measures time in beacon intervals */
                ZB_SCHEDULE_APP_ALARM((zb_callback_t)bdb_start_top_level_commissioning, ZB_BDB_NETWORK_STEERING, ZB_MILLISECONDS_TO_BEACON_INTERVAL(1000)); /* after delay call this function */
            }
            break;

        /* if an end device leaves the zigbee network */
        case ZB_ZDO_SIGNAL_LEAVE:
            LOG_INF("Reconnecting the network...");
            ZB_SCHEDULE_APP_ALARM((zb_callback_t)bdb_start_top_level_commissioning, ZB_BDB_NETWORK_STEERING, ZB_MILLISECONDS_TO_BEACON_INTERVAL(1000)); /* after delay call this function */
            break;

        default:
            ZB_ERROR_CHECK(zigbee_default_signal_handler(bufid));
            break;
    }
    if (bufid) {
        zb_buf_free(bufid);
    }
}

int main(void){
    LOG_INF("Starting Zigbee Light Bulb (Sleepy End Device)");
    gpio_pin_configure_dt(&led, GPIO_OUTPUT_INACTIVE); /* configure led gpio as output*/
    
    ZB_ZCL_REGISTER_DEVICE_CB(zcl_device_cb); /* tell zboss which function ahould be called as an event handler callback */
    ZB_AF_REGISTER_DEVICE_CTX(&light_bulb_ctx); /* register device context */
    app_clusters_attr_init(); /* attribute init function */
    
    zb_set_ed_timeout(ED_AGING_TIMEOUT_64MIN); /* set end device waiting timeout - if no reaction for 64 minutes -> end device is dead */
    zb_set_keepalive_timeout(ZB_MILLISECONDS_TO_BEACON_INTERVAL(30000)); /* set a keepalive timeout - how often an end device contacts parent to say it's alive */
    zigbee_configure_sleepy_behavior(true); /* enable sleepy behavoir */

    zb_zdo_pim_set_long_poll_interval(3000); /* how often an end device wakes up to ask a parent about a new message  */
    zigbee_enable(); /* enable zigbee */
    k_sleep(K_FOREVER); /* sleep forever*/
    return 0;
}
