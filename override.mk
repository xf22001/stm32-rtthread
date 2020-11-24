#
#
#================================================================
#   
#   
#   文件名称：override.mk
#   创 建 者：肖飞
#   创建日期：2020年04月02日 星期四 16时40分02秒
#   修改日期：2020年11月24日 星期二 09时43分36秒
#   描    述：
#
#================================================================
# list of objects
OBJECTS = $(addprefix $(BUILD_DIR)/,$(C_SOURCES:.c=.o))
# list of ASM program objects
OBJECTS += $(addprefix $(BUILD_DIR)/,$(ASM_SOURCES:.s=.o))
OBJECTS += $(addprefix $(BUILD_DIR)/,$(ASM_SOURCES_S:.S=.o))

USER_OBJECTS = $(addprefix $(BUILD_DIR)/,$(USER_C_SOURCES:.c=.o))

$(BUILD_DIR)/%.o: %.c Makefile | $(BUILD_DIR) 
	mkdir -p $(dir $@)
	$(CC) -c $(subst -MMD,-MD,$(CFLAGS)) -Wa,-a,-ad,-alms=$(BUILD_DIR)/$(<:.c=.lst) $< -o $@

$(BUILD_DIR)/%.o: %.s Makefile | $(BUILD_DIR)
	mkdir -p $(dir $@)
	$(AS) -c $(subst -MMD,-MD,$(CFLAGS)) $< -o $@

$(BUILD_DIR)/%.o: %.S Makefile | $(BUILD_DIR)
	mkdir -p $(dir $@)
	$(AS) -c $(subst -MMD,-MD,$(CFLAGS)) $< -o $@

$(USER_OBJECTS): user.mk

DEPS := $(OBJECTS:.o=.d)
-include $(DEPS)
