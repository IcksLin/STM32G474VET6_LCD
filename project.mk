# Project-owned build extensions.  Do not add these entries to the CubeMX
# generated Makefile; keep all custom source/include changes in this file.

PROJECT_C_SOURCES := \
Code/Hardwares/lcd_hw.c \
Code/Hardwares/lcd_user.c \
Code/Hardwares/lcd_fonts.c \
Code/System/delay.c

PROJECT_C_INCLUDES := \
-ICode/Hardwares \
-ICode/System

PROJECT_OBJECTS := $(addprefix $(BUILD_DIR)/,$(notdir $(PROJECT_C_SOURCES:.c=.o)))

C_SOURCES += $(PROJECT_C_SOURCES)
C_INCLUDES += $(PROJECT_C_INCLUDES)

vpath %.c $(sort $(dir $(PROJECT_C_SOURCES)))

# The generated ELF rule expands its prerequisite list while Makefile is read.
# Add custom objects explicitly so incremental builds remain correct.  Its
# existing link recipe expands OBJECTS when executed and therefore links them.
$(BUILD_DIR)/$(TARGET).elf: $(PROJECT_OBJECTS)
