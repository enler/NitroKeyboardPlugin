.SECONDEXPANSION:

include ../common/common.mk

OUTPUT      := keyboard_module
OUTPUT_ELF  := ../$(OUTPUT).elf
OUTPUT_KMOD := ../keyboard.kmod
INC_DIRS := ../overlay/include $(COMMON_INC_DIRS)
MODULE_OBJ_DIR := $(OBJ_DIR)/keyboard_module
HOST_SYMBOLS_ELF := ../$(OVERLAY_NAME).elf

LOCAL_C_SRCS := $(filter-out src/keyborad_pinyin.c src/keyborad_pinyin_ex.c,$(wildcard src/*.c))
ifeq ($(ENABLE_KEYBOARD_PINYIN_EX),1)
LOCAL_C_SRCS += src/keyborad_pinyin_ex.c
else
LOCAL_C_SRCS += src/keyborad_pinyin.c
endif
LOCAL_ASM_SRCS := $(wildcard src/*.S)

C_OBJS := $(addprefix $(MODULE_OBJ_DIR)/,$(notdir $(LOCAL_C_SRCS:.c=.o)))
ASM_OBJS := $(addprefix $(MODULE_OBJ_DIR)/,$(notdir $(LOCAL_ASM_SRCS:.S=.o)))
OBJS := $(ASM_OBJS) $(C_OBJS)
ALL_OBJS := $(OBJS) $(NITRO_LIB_OBJS)
DEPS := $(ALL_OBJS:%.o=%.d)

CFLAGS := -g -w -Os -std=gnu99 -ffreestanding -mabi=aapcs -march=armv5te -mtune=arm946e-s -mthumb -mthumb-interwork -mlong-calls $(foreach dir,$(INC_DIRS),-I $(dir)) -DARM9 -D__NDS__ -D__thumb__ -D__ARM_ARCH=5 -DNITROSDK_VER=$(NITROSDK_VER) -DENABLE_KEYBOARD_PINYIN_EX=$(ENABLE_KEYBOARD_PINYIN_EX)
LDFLAGS := -nostartfiles -nodefaultlibs -Ttext=0 -T linker.ld -T $(SYMBOLS_LD) -Wl,--just-symbols=$(HOST_SYMBOLS_ELF) -Wl,--use-blx -Wl,--emit-relocs -Wl,--wrap=glInit,--wrap=glResetTextures

$(shell mkdir -p $(MODULE_OBJ_DIR) $(sort $(dir $(NITRO_LIB_OBJS))))

.PHONY: all clean

all: $(OUTPUT_KMOD)
	@:

-include $(DEPS)

$(OBJS): Makefile $(BUILD_CONFIG_FILE)

$(MODULE_OBJ_DIR)/%.o: src/%.c
	$(CC) -MMD -MP -MF $(MODULE_OBJ_DIR)/$*.d $(CFLAGS) -o $@ -c $<

$(MODULE_OBJ_DIR)/%.o: src/%.S
	$(CC) -MMD -MP -MF $(MODULE_OBJ_DIR)/$*.d -D__ASSEMBLY__ $(CFLAGS) -o $@ -c $<

$(OUTPUT_ELF): $(OBJS) $(NITRO_LIB) $(HOST_SYMBOLS_ELF)
	$(LD) $(LDFLAGS) -o $@ $(OBJS) $(NITRO_LIB) $(LIBS)

$(OUTPUT_KMOD): $(OUTPUT_ELF) ../script/create_kmod.py
	python3 ../script/create_kmod.py $< $@

clean:
	rm -rf $(MODULE_OBJ_DIR)
	rm -f $(OUTPUT_ELF) $(OUTPUT_KMOD)
