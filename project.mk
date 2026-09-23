# Project-owned build extensions.  Do not add these entries to the CubeMX
# generated Makefile; keep all custom source/include changes in this file.

PROJECT_C_SOURCES := \
Code/Hardwares/lcd_hw.c \
Code/Hardwares/menu_key.c \
Code/Hardwares/zdt_x57.c \
Code/System/interrupt_callback.c \
Code/System/delay.c \
Code/GUI/app_lvgl.c \
Code/GUI/lv_port_disp.c \
Code/GUI/lv_port_indev.c

PROJECT_C_INCLUDES := \
-ICode/Hardwares \
-ICode/System \
-ICode/GUI

# LVGL v9.x vendored under Middlewares/lvgl.  Compile LVGL's own sources
# (excluding optional src/libs) and use a lv_conf.h from Middlewares/.
LVGL_DIR := Middlewares/lvgl

LVGL_SOURCES := $(filter-out $(LVGL_DIR)/src/libs/%, \
  $(wildcard $(LVGL_DIR)/src/*.c) \
  $(wildcard $(LVGL_DIR)/src/*/*.c) \
  $(wildcard $(LVGL_DIR)/src/*/*/*.c) \
  $(wildcard $(LVGL_DIR)/src/*/*/*/*.c) \
  $(wildcard $(LVGL_DIR)/src/*/*/*/*/*.c))

LVGL_C_INCLUDES := \
-I$(LVGL_DIR) \
-I$(LVGL_DIR)/include \
-I$(LVGL_DIR)/include/lvgl \
-IMiddlewares

LVGL_C_DEFS := \
-DLV_CONF_INCLUDE_SIMPLE \
-DLV_LVGL_H_INCLUDE_SIMPLE

PROJECT_OBJECTS := $(addprefix $(BUILD_DIR)/,$(notdir $(PROJECT_C_SOURCES:.c=.o)))
LVGL_OBJECTS := $(addprefix $(BUILD_DIR)/,$(notdir $(LVGL_SOURCES:.c=.o)))

C_SOURCES += $(PROJECT_C_SOURCES) $(LVGL_SOURCES)
C_INCLUDES += $(PROJECT_C_INCLUDES) $(LVGL_C_INCLUDES)
C_DEFS += $(LVGL_C_DEFS)

vpath %.c $(sort $(dir $(PROJECT_C_SOURCES)) $(dir $(LVGL_SOURCES)))

# The generated ELF rule expands its prerequisite list while Makefile is read.
# Add custom objects explicitly so incremental builds remain correct.  Its
# existing link recipe expands OBJECTS when executed and therefore links them.
$(BUILD_DIR)/$(TARGET).elf: $(PROJECT_OBJECTS) $(LVGL_OBJECTS)
