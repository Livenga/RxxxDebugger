EABI    = arm-none-eabi
CC      = $(EABI)-gcc
AS   	  = $(EABI)-as
OBJCOPY = $(EABI)-objcopy

OBJDIR = objs
PRJC   = uerr

ARCH   = armv6-m
CORTEX = cortex-m0


C_SRC  = $(shell find src/ -name \*.c)
AS_SRC = $(shell find src/ -name \*.s)

C_OBJS  = $(addprefix $(OBJDIR)/,$(patsubst %.c,%.o,$(C_SRC)))
AS_OBJS = $(addprefix $(OBJDIR)/,$(patsubst %.s,%.o,$(AS_SRC)))

OBJDIRS = $(sort $(dir $(C_OBJS) $(AS_OBJS)))


.PHONY: default to_hex clean all


default:
	[ -d $(OBJDIR) ] || mkdir -v $(OBJDIR)
	[ -d "$(OBJDIRS)" ] || mkdir -pv $(OBJDIRS)
	make $(PRJC).elf


$(PRJC).elf:$(C_OBJS) $(AS_OBJS)
	$(CC) -o $@ $^ \
		-T stm32f042.ld \
		-march=$(ARCH) -mcpu=$(CORTEX) -mtune=$(CORTEX) \
		-mfloat-abi=soft -msoft-float \
		-mthumb \
		-specs=nosys.specs \
		-lc -lnosys \
		-g


$(OBJDIR)/%.o:%.c
	$(CC) -o $@ -c $< \
		-marm \
		-march=$(ARCH) -mcpu=$(CORTEX) -mtune=$(CORTEX) \
		-mfloat-abi=soft -msoft-float \
		-mthumb \
		-g -Wunused-variable \
		-D ENABLE_RANDOM
	# -D ENABLE_REJECT_NL

$(OBJDIR)/%.o:%.s
	$(AS) -o $@ $< \
		-march=$(ARCH) \
		-mcpu=$(CORTEX) \
		-mfloat-abi=soft \
		-mthumb


to_hex:
	$(OBJCOPY) --output-target=binary \
		$(PRJC).elf $(PRJC).bin
	$(OBJCOPY) --output-target=ihex \
		$(PRJC).elf $(PRJC).hex

clean:
	[ ! -f $(PRJC).elf ] || rm -v $(PRJC).elf
	[ ! -d $(OBJDIR) ] || rm -rv $(OBJDIR)

all:
	make clean default to_hex
