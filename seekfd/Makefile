include compiler.mk

OBJDIR  := objs
PRJC     = seekfd

SRC  		= $(shell find src/ -name \*.c)
OBJS 		= $(addprefix $(OBJDIR)/,$(patsubst %.c,%.o,$(SRC)))
OBJDIRS = $(sort $(dir $(OBJS)))


DEFINES  =
DEFINES += -D__REQUIRED_ROOT__
DEFINES += -D__DEBUG__
#DEFINES += -D__ENABLE_DISPLAY_ALL__


.PHONY: default all clean


default:
	[ -d  $(OBJDIR)   ] || mkdir -v $(OBJDIR)
	[ -d "$(OBJDIRS)" ] || mkdir -pv $(OBJDIRS)
	make $(PRJC)


$(PRJC):$(OBJS)
	$(CC) -o $@ $^ \
		-Wall \
		-pthread


$(OBJDIR)/%.o:%.c
	$(CC) -o $@ -c $< -g \
		-Wall \
		$(DEFINES)


all:
	make clean default


clean:
	[ ! -f $(PRJC)   ] || rm -v $(PRJC)
	[ ! -d $(OBJDIR) ] || rm -rv $(OBJDIR)
