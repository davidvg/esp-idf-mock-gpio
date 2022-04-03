#include "CppUTest/TestHarness.h"
#include "CppUTestExt/MockSupport.h"
#include <stdio.h>

extern "C"
{
    #include "CppUTestExt/MockSupport_c.h"
    #include "driver/gpio.h"
    #include "gpio_switch.h"
}

// Mocked Functions ------------------------------------------------------------
esp_err_t gpio_config(const gpio_config_t *cfg)
{
    mock_c()->actualCall("gpio_config")
            ->withParameterOfType("gpio_config_t", "cfg", cfg);
    return mock_c()->intReturnValue();
}

/**
 * - gpio_num is a #DEFINE'd value equivalent to Int
 * - level is a uint32_t, equivalent to UnsignedInt
 */
esp_err_t gpio_set_level(gpio_num_t gpio_num, uint32_t level)
{
    mock_c()->actualCall("gpio_set_level")
            ->withIntParameters("gpio_num", gpio_num)
            ->withUnsignedIntParameters("level", level);
    return mock_c()->intReturnValue();
}

// Point object comparator -----------------------------------------------------
class GpioConfigComparator : public MockNamedValueComparator
{
    public:
    virtual bool isEqual(const void* object1, const void* object2)
    {
        const gpio_config_t* obj1 = (const gpio_config_t*)object1;
        const gpio_config_t* obj2 = (const gpio_config_t*)object2;

        bool res;
        res = obj1->mode == obj2->mode;
        res = res && (obj1->pin_bit_mask == obj2->pin_bit_mask);
        res = res && (obj1->pull_up_en == obj2->pull_up_en);
        res = res && (obj1->pull_down_en == obj2->pull_down_en);
        res = res && (obj1->intr_type == obj2->intr_type);
        return res;
    }
    virtual SimpleString valueToString(const void* object)
    {
        return StringFrom(((const gpio_config_t*)object)->pin_bit_mask);
    }
};

// Tests -----------------------------------------------------------------------
TEST_GROUP(GpioSwitch)
{
    GpioConfigComparator comp;
    int pin = 1; // Could be defined as: gpio_num_t = (gpio_num_t)1;

    gpio_config_t cfg = 
    {
        .pin_bit_mask = 1ULL << pin,
        .mode = GPIO_MODE_OUTPUT,
        .pull_up_en = GPIO_PULLUP_DISABLE,
        .pull_down_en = GPIO_PULLDOWN_DISABLE,
        .intr_type = GPIO_INTR_DISABLE,
    };

    void setup()
    {
        mock().installComparator("gpio_config_t", comp);
    }

    void teardown()
    {
        mock().checkExpectations();
        mock().clear();
        mock().removeAllComparatorsAndCopiers();
    }
};

TEST(GpioSwitch, ConfigureOutput)
{
    mock().expectOneCall("gpio_config")
          .withParameterOfType("gpio_config_t", "cfg", &cfg)
          .andReturnValue(ESP_OK);

    mock().expectOneCall("gpio_set_level")
          .withIntParameter("gpio_num", pin)
          .withUnsignedIntParameter("level", 0)
          .andReturnValue(ESP_OK);

    configure_output(pin);
}