#include <zephyr/kernel.h>
#include <zephyr/drivers/gpio.h>
#include <zephyr/sys/printk.h>

#define BTN_NODE(idx) DT_ALIAS(sw##idx)

static const struct gpio_dt_spec btns[] = {
#if DT_NODE_HAS_STATUS(BTN_NODE(0), okay)
    GPIO_DT_SPEC_GET(BTN_NODE(0), gpios),
#endif
#if DT_NODE_HAS_STATUS(BTN_NODE(1), okay)
    GPIO_DT_SPEC_GET(BTN_NODE(1), gpios),
#endif
#if DT_NODE_HAS_STATUS(BTN_NODE(2), okay)
    GPIO_DT_SPEC_GET(BTN_NODE(2), gpios),
#endif
#if DT_NODE_HAS_STATUS(BTN_NODE(3), okay)
    GPIO_DT_SPEC_GET(BTN_NODE(3), gpios),
#endif
};

static struct gpio_callback btn_cb_data;

static void button_cb(const struct device *port, struct gpio_callback *cb, uint32_t pins)
{
    for (int i = 0; i < ARRAY_SIZE(btns); i++) {
        if (pins & BIT(btns[i].pin)) {
            /* SW1 maps to sw0, SW2 -> sw1, etc. */
            printk("Pressed: SW%d\n", i + 1);
        }
    }
}

void main(void)
{
    /* Configure each button and enable interrupt */
    for (int i = 0; i < ARRAY_SIZE(btns); i++) {
        if (!device_is_ready(btns[i].port)) {
            printk("GPIO port not ready\n");
            return;
        }
        /* Buttons on Nordic DKs are active-low with pull-up */
        if (gpio_pin_configure_dt(&btns[i], GPIO_INPUT | GPIO_PULL_UP)) {
            printk("cfg fail\n"); return;
        }
        if (gpio_pin_interrupt_configure_dt(&btns[i], GPIO_INT_EDGE_TO_ACTIVE)) {
            printk("irq fail\n"); return;
        }
    }

    /* Register one shared callback for all button pins on the same port(s) */
    uint32_t pin_mask = 0;
    for (int i = 0; i < ARRAY_SIZE(btns); i++) {
        pin_mask |= BIT(btns[i].pin);
    }
    gpio_init_callback(&btn_cb_data, button_cb, pin_mask);

    /* Add callback to each involved port (handle split across ports) */
    /* Simpler: add per unique port */
    const struct device *seen[4] = {0}; int seen_n = 0;
    for (int i = 0; i < ARRAY_SIZE(btns); i++) {
        bool already = false;
        for (int j = 0; j < seen_n; j++) if (seen[j] == btns[i].port) already = true;
        if (!already) {
            gpio_add_callback(btns[i].port, &btn_cb_data);
            seen[seen_n++] = btns[i].port;
        }
    }

    printk("Ready: press SW1–SW4\n");
    while (1) {
        k_sleep(K_SECONDS(1));
    }
}
