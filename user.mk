#
#
#================================================================
#   
#   
#   文件名称：user.mk
#   创 建 者：肖飞
#   创建日期：2019年10月25日 星期五 13时04分38秒
#   修改日期：2020年11月24日 星期二 09时42分31秒
#   描    述：
#
#================================================================
include config.mk
include src.mk

USER_C_INCLUDES += $(CPPPATHS)

C_INCLUDES += $(USER_C_INCLUDES)

USER_C_SOURCES += $(SRC_FILES)

C_SOURCES += $(USER_C_SOURCES)

USER_ASM_SOURCES += $(ASM_FILES)
ASM_SOURCES_S += $(USER_ASM_SOURCES)

USER_CFLAGS += $(DEFINES)
CFLAGS += $(USER_CFLAGS)

LDFLAGS += -u _printf_float

#IAP_FILE := apps/modules/os/iap.h

#define update-iap-include
#	if [ -f $(IAP_FILE) ]; then
#		touch $(IAP_FILE);
#	fi
#endef

#ifeq ("$(origin APP)", "command line")
#CFLAGS += -DUSER_APP
#LDSCRIPT = STM32F407VETx_FLASH_APP.ld
#$(info $(shell $(update-iap-include)))
#$(info "build app!")
#else
#LDSCRIPT = STM32F407VETx_FLASH.ld
#$(info $(shell $(update-iap-include)))
#$(info "build bootloader!")
#endif

default: all

cscope: all
	rm cscope e_cs -rf
	mkdir -p cscope
	#$(silent)tags.sh prepare;
	$(silent)touch dep_files;
	$(silent)for f in $$(find . -type f -name "*.d" 2>/dev/null); do \
		for i in $$(cat "$$f" | sed 's/^.*://g' | sed 's/[\\ ]/\n/g' | sort -h | uniq); do \
			if test "${i:0:1}" = "/";then \
				echo "$i" >> dep_files; \
			else \
				readlink -f "$$i" >> dep_files; \
			fi; \
		done; \
	done;
	$(silent)cat dep_files | sort | uniq | sed 's/^\(.*\)$$/\"\1\"/g' >> cscope/cscope.files;
	$(silent)cat dep_files | sort | uniq >> cscope/ctags.files;
	$(silent)rm dep_files
	$(silent)tags.sh cscope;
	$(silent)tags.sh tags;
	$(silent)tags.sh env;

clean: clean-cscope
clean-cscope:
	rm cscope e_cs -rf
