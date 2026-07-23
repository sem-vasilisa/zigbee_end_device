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