#include <zboss_api.h> // the Zigbee stack itself (ZBOSS) - joining network, sending packets, etc.
#include <zephyr/kernel.h>  // Zephyr RTOS - gives you k_sleep, threads, etc.
#include "zb_sensor_node.h"
#include <zboss_api_addons.h> // extra helper macros/functions on top of zboss_api.h
#include <zephyr/logging/log.h>
#include <zephyr/drivers/gpio.h>
#include <zigbee/zigbee_app_utils.h> // Nordic's helper functions (zigbee_enable, sleepy behavior, etc.)
#include <zigbee/zigbee_error_handler.h> // ZB_ERROR_CHECK macro
#include <zephyr/drivers/sensor.h> /* gives access to zephyr sensor driver api */
#include <zephyr/drivers/i2c.h>
#include <zephyr/sys/crc.h>

LOG_MODULE_REGISTER(btz, LOG_LEVEL_INF);

#define LIGHT_BULB_ENDPOINT 10
#define LED_NODE DT_ALIAS(led0)

static const struct gpio_dt_spec led = GPIO_DT_SPEC_GET(LED_NODE, gpios);

static const struct i2c_dt_spec sts4x = I2C_DT_SPEC_GET(DT_NODELABEL(sts4x)); /* --- temperature sensor --- */

/* --- motion sensor --- */
static const struct device *bmi270_dev = DEVICE_DT_GET(DT_NODELABEL(bmi270));

struct zb_device_ctx{
    zb_zcl_basic_attrs_ext_t basic_attr;
    zb_zcl_identify_attrs_t identify_attr;
    zb_zcl_on_off_attrs_t on_off_attr;
    zb_zcl_temp_measurement_attrs_t temp_attr; /* temperature attribute */
};

static struct zb_device_ctx dev_ctx;

static void app_clusters_attr_init(void){
    dev_ctx.basic_attr.zcl_version = ZB_ZCL_VERSION;
    dev_ctx.basic_attr.power_source =  ZB_ZCL_BASIC_POWER_SOURCE_BATTERY;
    dev_ctx.identify_attr.identify_time = ZB_ZCL_IDENTIFY_IDENTIFY_TIME_DEFAULT_VALUE;
    dev_ctx.on_off_attr.on_off = ZB_FALSE;
    dev_ctx.temp_attr.measure_value = 0;  /* current temperature reading */
    dev_ctx.temp_attr.min_measure_value = -4000; /* the lowest temperature this sensor can report = -40.00 °C */
    dev_ctx.temp_attr.max_measure_value = 12500; /* the highest temperature this sensor can report = 125.00 °C */
    dev_ctx.temp_attr.tolerance = 20; /* ±0.20 °C */
}

/* attribute lists */
ZB_ZCL_DECLARE_BASIC_ATTRIB_LIST(basic_attrib_list, &dev_ctx.basic_attr.zcl_version, &dev_ctx.basic_attr.power_source);
ZB_ZCL_DECLARE_IDENTIFY_ATTRIB_LIST(identify_attr_list,&dev_ctx.identify_attr.identify_time);
ZB_ZCL_DECLARE_ON_OFF_ATTRIB_LIST(on_off_attr_list, &dev_ctx.on_off_attr.on_off);

// /* attribute list for the temperature */
ZB_ZCL_DECLARE_TEMP_MEASUREMENT_ATTRIB_LIST(temp_attr_list, &dev_ctx.temp_attr.measure_value, &dev_ctx.temp_attr.min_measure_value, &dev_ctx.temp_attr.max_measure_value, &dev_ctx.temp_attr.tolerance);

/* cluster lists */
ZB_DECLARE_LIGHT_BULB_CLUSTER_LIST(light_bulb_clusters, basic_attrib_list, identify_attr_list, on_off_attr_list, temp_attr_list); /* temp attr list was added */

/* endpoint */
ZB_DECLARE_LIGHT_BULB_EP(light_bulb_ep, LIGHT_BULB_ENDPOINT, light_bulb_clusters);

/* device context */
ZBOSS_DECLARE_DEVICE_CTX_1_EP(light_bulb_ctx, light_bulb_ep);

static void on_off_set_value(zb_bool_t value){
    dev_ctx.on_off_attr.on_off = value;
    if(value){
        LOG_INF("LED IS OFF!");
        gpio_pin_set_dt(&led, 1);
    }
    else{
        LOG_INF("LED IS ON!");
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

void zboss_signal_handler(zb_bufid_t bufid){
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

static int configure_motion(void){
    struct sensor_value odr = { .val1 = 100, .val2 = 0 };   /* 100 Hz */
    struct sensor_value fs  = { .val1 = 2,   .val2 = 0 };   /* +/- 2 g */
    int ret;

    ret = sensor_attr_set(bmi270_dev, SENSOR_CHAN_ACCEL_XYZ, SENSOR_ATTR_FULL_SCALE, &fs);
    
    if (ret) {
        LOG_ERR("bmi270 full-scale set failed: %d", ret);
        return ret;
    }

    ret = sensor_attr_set(bmi270_dev, SENSOR_CHAN_ACCEL_XYZ, SENSOR_ATTR_SAMPLING_FREQUENCY, &odr);
    
    if (ret) {
        LOG_ERR("bmi270 ODR set failed: %d", ret);
        return ret;
    }
    
    k_sleep(K_MSEC(50));
    return 0;
}

static void read_motion(void){
    struct sensor_value value_x, value_y, value_z;
    int ret;

    ret = sensor_sample_fetch(bmi270_dev);
    if (ret) {
        LOG_ERR("sensor_sample_fetch failed: %d", ret);
        return;
    }

    ret = sensor_channel_get(bmi270_dev, SENSOR_CHAN_ACCEL_X, &value_x);
    if (ret) {
        LOG_ERR("sensor_channel_get X failed: %d", ret);
        return;
    }

    ret = sensor_channel_get(bmi270_dev, SENSOR_CHAN_ACCEL_Y, &value_y);
    if (ret) {
        LOG_ERR("sensor_channel_get Y failed: %d", ret);
        return;
    }

    ret = sensor_channel_get(bmi270_dev, SENSOR_CHAN_ACCEL_Z, &value_z);
    if (ret) {
        LOG_ERR("sensor_channel_get Z failed: %d", ret);
        return;
    }

    LOG_INF("accel x=%d.%06d", value_x.val1, value_x.val2 < 0 ? -value_x.val2 : value_x.val2);
    LOG_INF("accel y=%d.%06d", value_y.val1, value_y.val2 < 0 ? -value_y.val2 : value_y.val2);
    LOG_INF("accel z=%d.%06d", value_z.val1, value_z.val2 < 0 ? -value_z.val2 : value_z.val2);
}

static void measure_and_report(zb_uint8_t param){
    int ret;

    if (!device_is_ready(sts4x.bus)) {
        LOG_ERR("STS4x I2C bus is not ready...");
    } else {
        uint8_t cmd = 0xFD;
        ret = i2c_write_dt(&sts4x, &cmd, 1);
        k_sleep(K_MSEC(10));
        uint8_t rx_buf[3];
        ret = i2c_read_dt(&sts4x, rx_buf, 3);
        uint8_t crc = crc8(rx_buf, 2, 0x31, 0xFF, false);
        if (crc == rx_buf[2]) {
            uint16_t raw = (rx_buf[0] << 8) | rx_buf[1];
            int64_t temp_milli_c = -45000 + (175000LL * (int64_t)raw) / 65535;
            zb_int16_t zcl_temp = (zb_int16_t)(temp_milli_c / 10);
            ZB_ZCL_SET_ATTRIBUTE(
                LIGHT_BULB_ENDPOINT,
                ZB_ZCL_CLUSTER_ID_TEMP_MEASUREMENT,
                ZB_ZCL_CLUSTER_SERVER_ROLE,
                ZB_ZCL_ATTR_TEMP_MEASUREMENT_VALUE_ID,
                (zb_uint8_t *)&zcl_temp,
                ZB_FALSE
            );
            LOG_INF("temperature = %lld.%03lld C", temp_milli_c / 1000, temp_milli_c % 1000);
        } else {
            LOG_ERR("STS4x CRC mismatch!");
        }
    }

    /* --- motion sensor reading --- */
    if (device_is_ready(bmi270_dev)) {
        LOG_INF("bmi270 is ready, reading now...");
        read_motion();
    } else {
        LOG_ERR("Sensor bmi270 is not ready...");
    }

    ZB_SCHEDULE_APP_ALARM(measure_and_report, 0, ZB_MILLISECONDS_TO_BEACON_INTERVAL(5000)); // shortened to 5s for testing
}

int main(void){

    LOG_INF("Starting Zigbee Light Bulb (Sleepy End Device)");
    gpio_pin_configure_dt(&led, GPIO_OUTPUT_INACTIVE); /* configure led gpio as output*/

    if (!device_is_ready(bmi270_dev)) {
        LOG_ERR("Sensor bmi270 is not ready...");
    } else {
        configure_motion();
    }

    ZB_ZCL_REGISTER_DEVICE_CB(zcl_device_cb); /* tell zboss which function ahould be called as an event handler callback */
    ZB_AF_REGISTER_DEVICE_CTX(&light_bulb_ctx); /* register device context */
    app_clusters_attr_init(); /* attribute init function */
    
    zb_set_ed_timeout(ED_AGING_TIMEOUT_64MIN); /* set end device waiting timeout - if no reaction for 64 minutes -> end device is dead */
    zb_set_keepalive_timeout(ZB_MILLISECONDS_TO_BEACON_INTERVAL(30000)); /* set a keepalive timeout - how often an end device contacts parent to say it's alive */
    zigbee_configure_sleepy_behavior(true); /* enable sleepy behavoir */

    zb_zdo_pim_set_long_poll_interval(3000); /* how often an end device wakes up to ask a parent about a new message  */
    zigbee_enable(); /* enable zigbee */

    ZB_SCHEDULE_APP_ALARM(measure_and_report, 0, ZB_MILLISECONDS_TO_BEACON_INTERVAL(1000)); // first run, 1s after scheduler starts

    k_sleep(K_FOREVER); /* sleep forever*/
    return 0;
}