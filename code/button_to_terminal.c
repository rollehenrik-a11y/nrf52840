#include <zephyr/kernel.h>
#include <zephyr/drivers/gpio.h>
#include <zephyr/sys/printk.h>

/* Button aliases on nrf52840dk_nrf52840: sw0..sw3 (SW1..SW4) */
#define BTN_NODE0 DT_ALIAS(sw0)  /* SW1 */
#define BTN_NODE1 DT_ALIAS(sw1)  /* SW2 */
#define BTN_NODE2 DT_ALIAS(sw2)  /* SW3 */
#define BTN_NODE3 DT_ALIAS(sw3)  /* SW4 */

/* Collect buttons in a tiny table (adjust if your board has fewer) */
static const struct gpio_dt_spec btns[] = {
    GPIO_DT_SPEC_GET(BTN_NODE0, gpios),
    GPIO_DT_SPEC_GET(BTN_NODE1, gpios),
    GPIO_DT_SPEC_GET(BTN_NODE2, gpios),
    GPIO_DT_SPEC_GET(BTN_NODE3, gpios),
};

static struct gpio_callback btn_cb;          /* one shared callback */
static uint32_t press_count[ARRAY_SIZE(btns)]; /* per-button counters */
static uint32_t press_total;                 /* total presses */

/* Simple debounce: ignore events within 30 ms for the same button */
#define DEBOUNCE_MS 30
static uint32_t last_evt_ms[ARRAY_SIZE(btns)];

static void on_button(const struct device *port,
                      struct gpio_callback *cb,
                      uint32_t pins)
{
    const uint32_t now = k_uptime_get_32();

    for (int i = 0; i < ARRAY_SIZE(btns); i++) {
        if (port == btns[i].port && (pins & BIT(btns[i].pin))) {
            /* Active-low buttons: pressed when level == 0 */
            int level = gpio_pin_get_dt(&btns[i]);
            if (level == 0) {
                if ((now - last_evt_ms[i]) < DEBOUNCE_MS) {
                    return; /* bounce; drop it */
                }
                last_evt_ms[i] = now;

                press_count[i]++;
                press_total++;
                printk("SW%d pressed  |  count=%u  (total=%u)\n",
                       i + 1, press_count[i], press_total);
            }
        }
    }
}

static void setup_buttons(void)
{
    /* Configure pins + enable interrupts on both edges (press/release) */
    uint32_t mask = 0;

    for (int i = 0; i < ARRAY_SIZE(btns); i++) {
        __ASSERT(device_is_ready(btns[i].port), "GPIO port not ready");

        /* Input with pull-up (DK buttons are active-low) */
        int err = gpio_pin_configure_dt(&btns[i], GPIO_INPUT | GPIO_PULL_UP);
        __ASSERT(err == 0, "cfg failed");

        /* Both edges: we count on press only (level==0 in callback) */
        err = gpio_pin_interrupt_configure_dt(&btns[i], GPIO_INT_EDGE_BOTH);
        __ASSERT(err == 0, "irq cfg failed");

        mask |= BIT(btns[i].pin);
    }

    /* One callback registered to the first button's port is fine on this DK,
       because all buttons live on the same GPIO port. If you split ports,
       add a callback per unique port. */
    gpio_init_callback(&btn_cb, on_button, mask);
    gpio_add_callback(btns[0].port, &btn_cb);
}

void main(void)
{
    printk("Ready. Press SW1–SW4 (active-low). Counting presses…\n");
    setup_buttons();

    while (1) {
        k_sleep(K_FOREVER); /* interrupt-driven; nothing to do here */
    }
}
