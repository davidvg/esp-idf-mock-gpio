# Mocking the ESP-IDF Gpio module

This repository contains a simple example mocking the ESP32 `driver/gpio.h` module in unit tests, though it's applicable to any other module.

Unit tests are done using [CppUTest](https://cpputest.github.io/) as the testing framework.

## Description

Mocking a function means substituting this function for a fake version, so we can control it's behaviour (generate desired errors, etc). In addition to this, we can:

- define how many times it's expected to be called, with which arguments, and which value we expect to be returned by the function.
- kee track of how many times it's actually been called, with which arguments and which value it returns.

In other words, we write *expectations* of what we expect to happen, call the actual code and check if those expectations have been fulfilled.

CppUTest has mocking capabilities and can check the expectations about the arguments passed to the functions when they are of type `int`, `double`, `const char*` or `void*`. When we try to mock functions from `gpio.h` we find ourselves passing as arguments more complex types.

For intance, when we configure a GPIO pin we need to pass a `gpio_config_t` struct, and CppUTest has no way to know if the passed argument is the correct type or value.

This example shows how to create mocks that extend this behaviour to these types.

## Example configuration

### ESP-IDF configuration

The example is configured as a standard ESP-IDF project. Once cloned, set the target and reconfigure the project. This will generate the `sdkconfig` file, needed by `Makefile`.

```
idf.py set-target esp32s3  # Use your target board
idf.py reconfigure
```

The project consists of an empty main file (`main/gpio_mock.c`), that in a normal project would contain the actual code, and a new component, `components/gpio_switch`, containing the module that will be tested. This module will contain a single function, `configure_output(pin)`, that will call the functions from `driver/gpio.h`. This calls to `gpio.h` are the one we will mock.

### CppUTest configuration

In addition to the standard file structure for an ESP-IDF project, there is on3 more folder, `tests`, where our unit tests are located, and a `Makefile` to run the testing framework.

The `Makefile` is an extended version of the original file used in the CppUTest examples, including the paths to the ESP-IDF header files. You may need to modify the line containing the location of the ESP-IDF; in my case, this is:

```txt
ESP_IDF_ROOT=${HOME}/src/esp-idf
```

## Test description

We will test a function from `gpio_switch` called `configure_output(pin)` that will configure a GPIO as an output and set it to LOW. So, we will be using two functions from `driver/gpio.h`:

```c
esp_err_t gpio_config(const gpio_config_t *cfg);
esp_err_t gpio_set_level(gpio_num_t gpio_num, uint32_t level);
```

We can see that these functions use custom ESP32 arguments: a pointer to a `gpio_config_t` struct, and a `gpio_num_t` that is equivalent to a `int`. The `uint32_t` argument is already handled bt CppUTest.

The actual `configure_output()` function is:

```c
void configure_output(int pin) {
    gpio_config_t cfg =  {
        .pin_bit_mask = 1ULL << (gpio_num_t)pin,
        .mode = GPIO_MODE_OUTPUT,
        .pull_up_en = GPIO_PULLUP_DISABLE,
        .pull_down_en = GPIO_PULLDOWN_DISABLE,
        .intr_type = GPIO_INTR_DISABLE,
    };
    gpio_config(&cfg);
    gpio_set_level(pin, 0);
}
```

### Defining the expectations

We write our tests, in which we create our expected calls to the `gpio.h` mocked functions:

```c
TEST(GpioSwitch, ConfigureOutput){
    mock().expectOneCall("gpio_config")
          .withParameterOfType("gpio_config_t", "cfg", &cfg)
          .andReturnValue(ESP_OK);

    mock().expectOneCall("gpio_set_level")
          .withIntParameter("gpio_num", pin)
          .withUnsignedIntParameter("level", 0)
          .andReturnValue(ESP_OK);

    configure_output(pin);
    // Here we would add more checks if needed. For instance, if a method
    // returns a value or a struct is modified inside the production code we
    // could check it here.
}
```

Here we tell CppUTest to expect one call to `gpio_config` with:

- one parameter of type `gpio_config_t`, which is a `struct` (actually a pointer to a `struct`)
- a return value of type `int` (`gpio_num_t` is just an integer number defined in `gpio.h`)

Also, we expect one call to `gpio_set_level` with:

- one parameter of type `int`, like before, called `gpio_num_t` and with value `gpio_num`.
- one parameter of type `unsigned int` called `level` and with value `level`.
- a return value of type `int` (`esp_err_t` is just a renamed `int`)

Here we are passing the names and values from the function signature.

Finally we call the code we want to test and that must fulfill the defined expectations.


### Mock version for the functions

We now write the fake version of these functions.

We start with `gpio_set_level()` as it uses types already handled by CppUTest:

```c
esp_err_t gpio_set_level(gpio_num_t gpio_num, uint32_t level) {
    mock_c()->actualCall("gpio_set_level")
            ->withIntParameters("gpio_num", gpio_num)
            ->withUnsignedIntParameters("level", level);
    return mock_c()->intReturnValue();
}
```

> It's important to completelly match the original prototype. For example, if we didn't include the `const` keyword the linker would throw an error for an undefined function `gpio_config`.

For `gpio_config()` we have a more complicated situation:

```c
esp_err_t gpio_config(const gpio_config_t *cfg) {
    mock_c()->actualCall("gpio_config")
            ->withParameterOfType("gpio_config_t", "cfg", cfg);
    return mock_c()->intReturnValue();
}
```

CppUTest has no way to check if the `gpio_config_t` argument is correct. If we ran the tests at this point with `make` we would obtain:

```txt
tests/GpioSwitchTests.cpp:92: error: Failure in TEST(GpioSwitch, ConfigureOutput)
	MockFailure: No way to compare type <gpio_config_t>. Please install a MockNamedValueComparator.

.
Errors (1 failures, 1 tests, 1 ran, 2 checks, 0 ignored, 0 filtered out, 0 ms)
```

CppUTests is telling we need to install a *comparator* for the `gpio_config_t` type.

### Creating the comparator

A comparator is a class that inherits from a generic interface that defines how to compare objects. We create our custom comparator for `gpio_config_t`:

```c++
class GpioConfigComparator : public MockNamedValueComparator {
    public:
    virtual bool isEqual(const void* object1, const void* object2) {
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
    virtual SimpleString valueToString(const void* object) {
        return StringFrom(((const gpio_config_t*)object)->pin_bit_mask);
    }
};
```

We just override two methods. The first one is `isEqual`, and inside it we cast the generic `const void* object` arguments to match our type, `gpio_config_t`. Next, we just need to compare both objects and check that they are equal. In this case, as it's a `struct`, we check that the members for both objects are equal.

The second method, `valueToString`, is related with a text returned for an error. We define a dummy return.

Now we only need to install this comparator inside `setup()`, and remove it inside `teardown()`, as well as check expectations and reset the framework for the next test:

```c++
TEST_GROUP(GpioSwitch)
{
    GpioConfigComparator comp;

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
```

Runing `make` in the project's root directory should run the existing test and pass it:

```txt
compiling AllTests.cpp
compiling GpioSwitchTests.cpp
compiling gpio_switch.c
Building archive lib/libGpioMock.a
a - objs/./components/gpio_switch/gpio_switch.o
Linking GpioMock_tests
Running GpioMock_tests
.
OK (1 tests, 1 ran, 4 checks, 0 ignored, 0 filtered out, 0 ms)
```