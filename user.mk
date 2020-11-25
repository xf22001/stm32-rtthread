#
#
#================================================================
#   
#   
#   文件名称：user.mk
#   创 建 者：肖飞
#   创建日期：2019年10月25日 星期五 13时04分38秒
#   修改日期：2020年11月25日 星期三 13时52分55秒
#   描    述：
#
#================================================================
include config.mk
include src.mk

CPPPATHS := $(filter-out -I$(BSP_ROOT)/Drivers, $(CPPPATHS))
CPPPATHS := $(filter-out -I$(BSP_ROOT)/Libraries%, $(CPPPATHS))
CPPPATHS := $(filter-out -I$(BSP_ROOT)/applications, $(CPPPATHS))
USER_C_INCLUDES += $(CPPPATHS)
USER_C_INCLUDES += -Iapps
USER_C_INCLUDES += -Iapps/drivers

C_INCLUDES += $(USER_C_INCLUDES)

SRC_FILES := $(filter-out $(BSP_ROOT)/Drivers%, $(SRC_FILES))
SRC_FILES := $(filter-out $(BSP_ROOT)/Libraries%, $(SRC_FILES))
SRC_FILES := $(filter-out $(BSP_ROOT)/applications%, $(SRC_FILES))

USER_C_SOURCES += $(filter-out %.S, $(SRC_FILES))
USER_C_SOURCES += apps/startup.c
USER_C_SOURCES += apps/application.c
USER_C_SOURCES += apps/app.c
USER_C_SOURCES += apps/drivers/usart.c
USER_C_SOURCES += apps/drivers/serial.c

C_SOURCES += $(USER_C_SOURCES)

USER_ASM_SOURCES += $(filter %.S, $(SRC_FILES))
ASM_SOURCES_S += $(USER_ASM_SOURCES)

USER_CFLAGS += $(DEFINES)
CFLAGS += $(USER_CFLAGS)

LDFLAGS += -u _printf_float

LDSCRIPT = stm32_rom.ld

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
