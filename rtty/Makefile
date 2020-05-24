EABI   = ../../../Impinj/ETK/octane_etk-6.0.0.240/arm-toolchain/bin/arm-none-linux-gnueabi
#CC     = $(EABI)-gcc
CC     = gcc
OBJDIR = objs
PRJC   = rtty

SRC  		= $(shell find src/ -name \*.c)
OBJS 		= $(addprefix $(OBJDIR)/,$(patsubst %.c,%.o,$(SRC)))
OBJDIRS = $(sort $(dir $(OBJS)))


.PHONY: default clean all


default:
	[ -d  $(OBJDIR)   ] || mkdir -v $(OBJDIR)
	[ -d "$(OBJDIRS)" ] || mkdir -pv $(OBJDIRS)
	make $(PRJC)


$(PRJC):$(OBJS)
	$(CC) -o $@ $^ \
		-Wall


$(OBJDIR)/%.o:%.c
	$(CC) -o $@ -c $< \
		-Wall


clean:
	[ ! -f $(PRJC)   ] || rm -v $(PRJC)
	[ ! -d $(OBJDIR) ] || rm -rv $(OBJDIR)


all:
	make clean default
