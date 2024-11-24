.SECONDEXPANSION:

ifeq ($(strip $(DEVKITARM)),)
$(error "Please set DEVKITARM in your environment. export DEVKITARM=<path to>devkitARM")
endif

include config

OUTPUT     := $(shell printf "overlay_%04d" $(OVERLAY_ID))
OUTPUT_ELF := $(OUTPUT).elf
OUTPUT_BIN := $(OUTPUT).bin
LOADER     := overlay_loader
LOADER_ELF := $(LOADER).elf
LOADER_BIN := $(LOADER).bin

BACKUP_DIR := backup_files
ARM9_BINARY_BACKUP := $(BACKUP_DIR)/arm9.bin
OVERLAY_TABLE_BACKUP := $(BACKUP_DIR)/overlay_table.bin

PREFIX  := $(DEVKITARM)/bin/arm-none-eabi-
CC      := $(PREFIX)gcc
OBJCOPY := $(PREFIX)objcopy
OBJDUMP := $(PREFIX)objdump
AS      := $(PREFIX)as
LD      := $(PREFIX)gcc
CC1     := $(shell $(CC) --print-prog-name=cc1) -quiet
CPP     := $(PREFIX)cpp
AR      := $(PREFIX)ar

LIBNDS  := $(DEVKITPRO)/libnds/lib
LIBPATH := -L "$(dir $(shell $(CC) -print-file-name=libgcc.a))" -L "$(dir $(shell $(CC) -print-file-name=libnosys.a))" -L "$(dir $(shell $(CC) -print-file-name=libc.a))"
LIBS	:= $(LIBPATH) -lc -lgcc

OBJ_DIR  := build
INC_DIRS := include nitro $(DEVKITARM)/include $(DEVKITARM)/../libnds/include

C_SRCS  := $(wildcard source/*.c)
ASM_SRCS:= $(wildcard source/*.S)
C_OBJS  := $(C_SRCS:%.c=$(OBJ_DIR)/%.o)
ASM_OBJS:= $(ASM_SRCS:%.S=$(OBJ_DIR)/%.o)
OBJS    := $(C_OBJS) $(ASM_OBJS)

OVERALY_LDR_ASM := overlay_ldr/hook_nitro_main.S
OVERLAY_LDR_SRC := overlay_ldr/overlay_ldr.c 
OVERLAY_LDR_C_OBJS := $(OVERLAY_LDR_SRC:%.c=$(OBJ_DIR)/%.o)
OVERLAY_LDR_OBJS := $(OVERALY_LDR_ASM:%.S=$(OBJ_DIR)/%.o) $(OVERLAY_LDR_C_OBJS)

ALL_OBJS := $(OVERLAY_LDR_OBJS) $(OBJS)
DEPS   := $(ALL_OBJS:%.o=%.d)

OVERLAY_SIZE_SYM_FILE := $(OBJ_DIR)/overlay_size.sym

CFLAGS	:= -g -w -Os -std=gnu99 -Os -ffreestanding -mabi=aapcs -march=armv5te -mthumb -mtune=arm946e-s -mthumb-interwork $(foreach dir,$(INC_DIRS),-I $(dir)) -DARM9=1
LDFLAGS := -nostartfiles -nodefaultlibs -Wl,--use-blx  -Wl,--wrap=glInit_C,--wrap=glResetTextures -T symbol.sym

$(OUTPUT_ELF) : LDFLAGS += -Ttext=$(OVERLAY_ADDR) -T linker.ld
$(LOADER_ELF) : CFLAGS += -DOVERLAY_ID=$(OVERLAY_ID)


$(shell mkdir -p $(sort $(dir $(ALL_OBJS))))

.PHONY: all clean apply backup create_backup_dir

all: apply
	@:

-include $(DEPS)

create_backup_dir:
	@if [ ! -d $(BACKUP_DIR) ]; then mkdir $(BACKUP_DIR); fi

backup: create_backup_dir
	@if [ ! -f $(ARM9_BINARY_BACKUP) ]; then cp $(ARM9_BINARY_PATH) $(ARM9_BINARY_BACKUP); fi
	@if [ ! -f $(OVERLAY_TABLE_BACKUP) ]; then cp $(OVERLAY_TABLE_PATH) $(OVERLAY_TABLE_BACKUP); fi

restore:
	@if [ -f $(ARM9_BINARY_BACKUP) ]; then cp $(ARM9_BINARY_BACKUP) $(ARM9_BINARY_PATH); fi
	@if [ -f $(OVERLAY_TABLE_BACKUP) ]; then cp $(OVERLAY_TABLE_BACKUP) $(OVERLAY_TABLE_PATH); fi

$(OBJ_DIR)/%.o: %.c
	$(CC) -MMD -MP -MF $(OBJ_DIR)/$*.d $(CFLAGS) -o $(OBJ_DIR)/$*.o -c $<

$(OBJ_DIR)/%.o: %.S
	$(CC) -D__ASSEMBLY__ $(CFLAGS) -c $< -o $@

$(LOADER_ELF): $(OVERLAY_LDR_OBJS) $(OUTPUT_BIN)
	@echo "OverlaySize = 0x$(shell $(OBJDUMP) -t $(OUTPUT_ELF) | grep -w '__size__' | awk '{print $$1}');" > $(OVERLAY_SIZE_SYM_FILE)
	$(LD) $(LDFLAGS) -Ttext=$(OVERLAY_LDR_ADDR) -T overlay_ldr.ld -T $(OVERLAY_SIZE_SYM_FILE) -o $@ $(OVERLAY_LDR_OBJS)

$(OUTPUT_ELF): $(OBJS)
	$(LD) $(LDFLAGS) -o $@ $^ $(LIBNDS)/libnds9.a $(LIBS)
	$(OBJDUMP) -t $@ | grep 'text' | grep 'g' | awk '{print $$1"\t"$$NF}' > rom/build.txt 

$(OUTPUT_BIN): $(OUTPUT_ELF)
	$(OBJCOPY) --set-section-flags .init_functions=alloc,load,contents -O binary $< $@

$(LOADER_BIN): $(LOADER_ELF)
	$(OBJCOPY) --set-section-flags .bss=alloc,load,contents -O binary $< $@

clean:
	rm -rf $(OBJ_DIR)
	rm *.bin *.elf

apply: $(LOADER_BIN) backup restore
	@echo "Applying arm9..."
	@python tools/inject_overlay_ldr.py $(ARM9_BINARY_PATH) $(LOADER_BIN) $(ARM9_LOAD_ADDR) $(OVERLAY_LDR_ADDR) $(Orig_NitroMain) \
	    0x$(shell $(OBJDUMP) -t $(LOADER_ELF) | grep -w 'Hook_NitroMain' | awk '{print $$1}')
	@echo "Applying overlay table..."
	@python tools/patch_overlay_table.py add-new-overlay $(OVERLAY_TABLE_PATH) $(OVERLAY_ID) $(OVERLAY_ADDR) \
		0x$(shell $(OBJDUMP) -t $(OUTPUT_ELF) | grep -w '__ram_size__' | awk '{print $$1}') \
		0x$(shell $(OBJDUMP) -t $(OUTPUT_ELF) | grep -w '__bss_size__' | awk '{print $$1}') \
		0x$(shell $(OBJDUMP) -t $(OUTPUT_ELF) | grep -w '__init_functions_begin__' | awk '{print $$1}') \
		0x$(shell $(OBJDUMP) -t $(OUTPUT_ELF) | grep -w '__init_functions_end__' | awk '{print $$1}')
	@python tools/patch_overlay_table.py modify-exist-overlay $(OVERLAY_TABLE_PATH) $(INJECT_OVERLAY_ID) \
        0x$(shell $(OBJDUMP) -t $(LOADER_ELF) | grep -w 'OverlayStaticInitFunc' | awk '{print $$1}')
	@echo "Copying overlay..."
	@cp $(OUTPUT_BIN) $(OVERLAY_DIR)
	@echo "Done!"




