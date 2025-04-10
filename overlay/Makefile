.SECONDEXPANSION:

include ../common/common.mk

OUTPUT     := $(OVERLAY_NAME)
OUTPUT_ELF := ../$(OUTPUT).elf
OUTPUT_BIN := ../$(OUTPUT).bin

INC_DIRS := include $(COMMON_INC_DIRS)

C_SRCS  := $(wildcard src/*.c)
ASM_SRCS:= $(wildcard src/*.S)
C_OBJS  := $(C_SRCS:%.c=$(OBJ_DIR)/%.o)
ASM_OBJS:= $(ASM_SRCS:%.S=$(OBJ_DIR)/%.o)
OBJS    := $(C_OBJS) $(ASM_OBJS)

ALL_OBJS := $(OBJS) $(NITRO_LIB_OBJS)
DEPS   := $(ALL_OBJS:%.o=%.d)

COMPILE_MODE := -mthumb
DEFS    := -DARM9 -D__NDS__ -D__thumb__ -D__ARM_ARCH=5 -DNITROSDK_VER=$(NITROSDK_VER)
CFLAGS  := -g -w -Os -std=gnu99 -ffreestanding -mabi=aapcs -march=armv5te -mtune=arm946e-s -mthumb-interwork $(foreach dir,$(INC_DIRS),-I $(dir))
LDFLAGS := -nostartfiles -nodefaultlibs -Ttext=$(OVERLAY_ADDR) -T linker.ld -T $(SYMBOLS_LD) -Wl,--use-blx -Wl,--wrap=glInit,--wrap=glResetTextures

$(OBJ_DIR)/src/overlay_entry.o : COMPILE_MODE := -marm

$(shell mkdir -p $(sort $(dir $(ALL_OBJS))))

.PHONY: all clean

all: $(OUTPUT_BIN)
	@:

-include $(DEPS)

$(OBJ_DIR)/%.o: %.c
	$(CC) -MMD -MP -MF $(OBJ_DIR)/$*.d $(CFLAGS) $(COMPILE_MODE) $(DEFS) -o $(OBJ_DIR)/$*.o -c $<

$(OBJ_DIR)/%.o: %.S
	$(CC) -D__ASSEMBLY__ $(CFLAGS) -c $< -o $@

$(OUTPUT_ELF): $(OBJS) $(NITRO_LIB)
	$(LD) $(LDFLAGS) -o $@ $^ $(NITRO_LIB) $(LIBS)

$(OUTPUT_BIN): $(OUTPUT_ELF)
	$(OBJCOPY) --set-section-flags .init_functions=alloc,load,contents -O binary $< $@

clean:
	rm -rf $(OBJ_DIR)
	rm $(OUTPUT_ELF) $(OUTPUT_BIN)



