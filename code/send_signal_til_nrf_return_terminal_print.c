#include <zephyr/kernel.h>
#include <zephyr/device.h>
#include <zephyr/drivers/gpio.h>
#include <zephyr/sys/printk.h>
#include <zephyr/bluetooth/bluetooth.h>
#include <zephyr/bluetooth/uuid.h>
#include <zephyr/bluetooth/gatt.h>

/* LED0 alias på nRF52840 DK */
#define LED0_NODE DT_ALIAS(led0)
#if !DT_NODE_HAS_STATUS(LED0_NODE, okay)
#error "Alias 'led0' ikke fundet i devicetree"
#endif
static const struct gpio_dt_spec led0 = GPIO_DT_SPEC_GET(LED0_NODE, gpios);

/* ====== UUIDs (128-bit) ====== */
static struct bt_uuid_128 alarm_svc_uuid =
    BT_UUID_INIT_128(BT_UUID_128_ENCODE(0x12345678, 0x1234, 0x5678, 0x1234, 0x56789abcdef0));

static struct bt_uuid_128 alarm_ctrl_uuid =
    BT_UUID_INIT_128(BT_UUID_128_ENCODE(0x12345678, 0x1234, 0x5678, 0x1234, 0x56789abcdef1));

/* ====== LED auto-off work ====== */
static void led_off_work_handler(struct k_work *work);
K_WORK_DELAYABLE_DEFINE(led_off_work, led_off_work_handler);

static void led_off_work_handler(struct k_work *work)
{
    gpio_pin_set_dt(&led0, 0);
}

/* ====== GATT write callback ====== */
static ssize_t alarm_ctrl_write(struct bt_conn *conn, const struct bt_gatt_attr *attr,
                                const void *buf, uint16_t len, uint16_t offset, uint8_t flags)
                                
{

    if (offset != 0) {
        return BT_GATT_ERR(BT_ATT_ERR_INVALID_OFFSET);
    }
    if (len != 1) {
        return BT_GATT_ERR(BT_ATT_ERR_INVALID_ATTRIBUTE_LEN);
    }

    const uint8_t v = ((const uint8_t *)buf)[0];

    printk("alarm_ctrl_write: len=%u, first=0x%02x\n",
           (unsigned)len, (unsigned)v);

    if (v == 0x01) {
        gpio_pin_set_dt(&led0, 1);
        k_work_reschedule(&led_off_work, K_SECONDS(5));
    } else {
        gpio_pin_set_dt(&led0, 0);
        k_work_cancel_delayable(&led_off_work);
    }
    return len;
}

/* ====== GATT tabel ====== */
BT_GATT_SERVICE_DEFINE(alarm_svc,
    BT_GATT_PRIMARY_SERVICE(&alarm_svc_uuid),
    BT_GATT_CHARACTERISTIC(&alarm_ctrl_uuid.uuid,
                           BT_GATT_CHRC_WRITE | BT_GATT_CHRC_WRITE_WITHOUT_RESP,
                           BT_GATT_PERM_WRITE,
                           NULL, alarm_ctrl_write, NULL)
);

/* ====== Extended Advertising setup ====== */
static int start_ext_advertising(void)
{
    /* AD: flags + 128-bit service-UUID
       Bemærk: 128-bit UUID skal ligge som rå 16 bytes i little-endian */
    const struct bt_data ad[] = {
        BT_DATA_BYTES(BT_DATA_FLAGS, (BT_LE_AD_GENERAL | BT_LE_AD_NO_BREDR)),
        BT_DATA(BT_DATA_UUID128_ALL, alarm_svc_uuid.val, 16),
    };

    /* Scan response: device name */
    const struct bt_data sd[] = {
        BT_DATA(BT_DATA_NAME_COMPLETE, CONFIG_BT_DEVICE_NAME,
                sizeof(CONFIG_BT_DEVICE_NAME) - 1),
    };

    /* Extended adv param (connectable, bruger identity, hurtig interval) */
    static const struct bt_le_adv_param adv_param =
        BT_LE_ADV_PARAM_INIT(
            BT_LE_ADV_OPT_CONNECTABLE | BT_LE_ADV_OPT_USE_IDENTITY,
            BT_GAP_ADV_FAST_INT_MIN_2,
            BT_GAP_ADV_FAST_INT_MAX_2,
            NULL); /* primary PHY auto */

    struct bt_le_ext_adv *adv = NULL;
    struct bt_le_ext_adv_start_param start = { 0 };

    int err = bt_le_ext_adv_create(&adv_param, NULL, &adv);
    if (err) {
        printk("bt_le_ext_adv_create err: %d\n", err);
        return err;
    }

    err = bt_le_ext_adv_set_data(adv, ad, ARRAY_SIZE(ad), sd, ARRAY_SIZE(sd));
    if (err) {
        printk("bt_le_ext_adv_set_data err: %d\n", err);
        return err;
    }

    err = bt_le_ext_adv_start(adv, &start);
    if (err) {
        printk("bt_le_ext_adv_start err: %d\n", err);
        return err;
    }

    printk("Extended advertising started\n");
    return 0;
}

/* ====== main ====== */
int main(void)
{
    int err;

    /* LED init */
    if (!device_is_ready(led0.port)) {
        printk("LED0 port ikke klar\n");
        return 0;
    }
    err = gpio_pin_configure_dt(&led0, GPIO_OUTPUT_INACTIVE);
    if (err) {
        printk("LED0 config fejl: %d\n", err);
        return 0;
    }

    /* BT on */
    err = bt_enable(NULL);
    if (err) {
        printk("bt_enable fejl: %d\n", err);
        return 0;
    }
    printk("Bluetooth enabled\n");

    /* Start extended advertising */
    err = start_ext_advertising();
    if (err) {
        printk("Advertising fejl: %d\n", err);
        return 0;
    }

    printk("GATT klar. Skriv 0x01 til control-char for LED i 5 sek.\n");
    /* Kør for evigt */
    for (;;) {
        k_sleep(K_SECONDS(1));
    }
    /* not reached */
}
