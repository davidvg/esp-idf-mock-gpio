#Set this to @ to keep the makefile quiet
ifndef SILENCE
	SILENCE = @
endif

COMPONENT_NAME = GpioMock

CPPUTEST_USE_EXTENSIONS = Y
CPP_PLATFORM = gcc

MEMORY_MAP_FILE = $(COMPONENT_NAME).map
#CPPUTEST_LDFLAGS=-Xlinker -Map=$(MEMORY_MAP_FILE)

# ESP-IDF specific paths
ESP_IDF_ROOT=${HOME}/src/esp-idf
ESP_IDF_SDKCONFIG_PATH=./build/config
ESP_IDF_COMPONENTS_PATH=${ESP_IDF_ROOT}/components

# To include ESP-IDF headers, pedanting errors must be disabled.
CPPUTEST_PEDANTIC_ERRORS=N

# This line is overriding the default new macros. This is helpful when using
#  std library includes like <list> and other containers so that memory leak
#  detection does not conflict with stl. The header file does two things:
#  - include <list>
#  - include MemoryLeakDetectorNewMacros.h, in $(CPPUTEST_HOME)/include/CppUTest
#  A new header file doing this two things should be created in an included path
# CPPUTEST_MEMLEAK_DETECTOR_NEW_MACRO_FILE = -include include/ExamplesNewOverrides.h
CPPUTEST_MEMLEAK_DETECTOR_NEW_MACRO_FILE = -include tests/ExamplesNewOverrides.h

SRC_DIRS = \
	./components/gpio_switch \
	
TEST_SRC_DIRS = \
	tests

INCLUDE_DIRS = \
  . \
  ./components/gpio_switch/include \
  $(SRC_DIRS) \
  ${ESP_IDF_SDKCONFIG_PATH} \
  ${ESP_IDF_COMPONENTS_PATH}/esp_hw_support/include \
  ${ESP_IDF_COMPONENTS_PATH}/driver/include \
  ${ESP_IDF_COMPONENTS_PATH}/esp_common/include \
  ${ESP_IDF_COMPONENTS_PATH}/soc/esp32s3/include \
  ${ESP_IDF_COMPONENTS_PATH}/soc/include \
  ${ESP_IDF_COMPONENTS_PATH}/hal/include \
  ${ESP_IDF_COMPONENTS_PATH}/esp_rom/include \

MOCKS_SRC_DIRS = 

include $(CPPUTEST_HOME)/build/MakefileWorker.mk
