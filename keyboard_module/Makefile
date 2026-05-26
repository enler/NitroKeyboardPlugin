.SECONDEXPANSION:

include ../common/common.mk

OUTPUT      := keyboard_module
OUTPUT_ELF  := ../$(OUTPUT).elf
OUTPUT_KMOD := ../keyboard.kmod
INC_DIRS := ../overlay/include $(COMMON_INC_DIRS)
MODULE_OBJ_DIR := $(OBJ_DIR)/keyboard_module
HOST_SYMBOLS_LD := $(MODULE_OBJ_DIR)/host_symbols.ld

LOCAL_C_SRCS := $(wildcard src/*.c)
LOCAL_ASM_SRCS := $(wildcard src/*.S)

C_OBJS := $(addprefix $(MODULE_OBJ_DIR)/,$(notdir $(LOCAL_C_SRCS:.c=.o)))
ASM_OBJS := $(addprefix $(MODULE_OBJ_DIR)/,$(notdir $(LOCAL_ASM_SRCS:.S=.o)))
OBJS := $(ASM_OBJS) $(C_OBJS)
ALL_OBJS := $(OBJS) $(NITRO_LIB_OBJS)
DEPS := $(ALL_OBJS:%.o=%.d)

CFLAGS := -g -w -Os -std=gnu99 -ffreestanding -mabi=aapcs -march=armv5te -mtune=arm946e-s -mthumb -mthumb-interwork -mlong-calls $(foreach dir,$(INC_DIRS),-I $(dir)) -DARM9 -D__NDS__ -D__thumb__ -D__ARM_ARCH=5 -DNITROSDK_VER=$(NITROSDK_VER)
LDFLAGS := -nostartfiles -nodefaultlibs -Ttext=0 -T linker.ld -T $(SYMBOLS_LD) -T $(HOST_SYMBOLS_LD) -Wl,--use-blx -Wl,--emit-relocs -Wl,--wrap=glInit,--wrap=glResetTextures

$(shell mkdir -p $(MODULE_OBJ_DIR) $(sort $(dir $(NITRO_LIB_OBJS))))

.PHONY: all clean

all: $(OUTPUT_KMOD)
	@:

-include $(DEPS)

$(MODULE_OBJ_DIR)/%.o: src/%.c
	$(CC) -MMD -MP -MF $(MODULE_OBJ_DIR)/$*.d $(CFLAGS) -o $@ -c $<

$(MODULE_OBJ_DIR)/%.o: src/%.S
	$(CC) -MMD -MP -MF $(MODULE_OBJ_DIR)/$*.d -D__ASSEMBLY__ $(CFLAGS) -o $@ -c $<

$(HOST_SYMBOLS_LD): ../$(OVERLAY_NAME).elf ../script/export_ld_symbols.py
	python3 ../script/export_ld_symbols.py $< $@

$(OUTPUT_ELF): $(OBJS) $(NITRO_LIB) $(HOST_SYMBOLS_LD)
	$(LD) $(LDFLAGS) -o $@ $(filter-out $(HOST_SYMBOLS_LD),$^) $(NITRO_LIB) $(LIBS)

$(OUTPUT_KMOD): $(OUTPUT_ELF) ../script/create_kmod.py
	python3 ../script/create_kmod.py $< $@

clean:
	rm -rf $(MODULE_OBJ_DIR)
	rm -f $(OUTPUT_ELF) $(OUTPUT_KMOD)
