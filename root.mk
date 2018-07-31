################################################################
# Root Makefile of the whole project
################################################################

# Basic Definitions
################################################################
ARCH		:= arm
BOARD		?= uwp566x_evb
BOOT		:= mcuboot
KERNEL		:= zephyr

# Directories and Files
################################################################
SHELL		:= /bin/bash
PWD		:= $(shell pwd)
PRJDIR		:= $(PWD)

app_DIR		:= $(PRJDIR)/app
boot_DIR	:= $(PRJDIR)/$(BOOT)
build_DIR	:= $(PRJDIR)/build
fdl_DIR		:= $(PRJDIR)/fdl
kernel_DIR	:= $(PRJDIR)/$(KERNEL)
toolchain_DIR	:= $(PRJDIR)/toolchain

kernel_BUILD_DIR	:= $(build_DIR)/$(KERNEL)

cmake_FILE	:= $(toolchain_DIR)/cmake-3.8.2-Linux-x86_64.sh
cmake_DIR	:= $(basename $(cmake_FILE))
export PATH	:=$(cmake_DIR)/bin:$(PATH)

sdk_FILE	:= $(toolchain_DIR)/zephyr-sdk-0.9.3-setup.run
sdk_DIR		:= $(basename $(sdk_FILE))
export ZEPHYR_TOOLCHAIN_VARIANT	:= zephyr
export ZEPHYR_SDK_INSTALL_DIR	:= $(sdk_DIR)

# Targets
################################################################
DEFAULT_TARGETS		:= cmake sdk kernel
CLEAN_TARGETS		:= $(addsuffix -clean,$(DEFAULT_TARGETS))
DISTCLEAN_TARGETS	:= $(addsuffix -distclean,$(DEFAULT_TARGETS))

.PHONY: all
all: $(DEFAULT_TARGETS)

.PHONY: clean
clean: $(CLEAN_TARGETS)

.PHONY: distclean
distclean: $(DISTCLEAN_TARGETS)
	@ if [ -h $(fdl_DIR) ]; then rm -f $(fdl_DIR); fi
	if [ -d $(DIST_DIR) ]; then rm -rf $(DIST_DIR); fi

# Macros
################################################################
# MESSAGE Macro -- display a message in bold type
MESSAGE = echo "\n$(TERM_BOLD)>>> $(1)$(TERM_RESET)"
TERM_BOLD := $(shell tput smso 2>/dev/null)
TERM_RESET := $(shell tput rmso 2>/dev/null)

# Macro of Extracting cmake
# $(1): cmake file
# $(2): cmake directory
define EXTRACT_CMAKE
	cd $(toolchain_DIR); \
	yes | sh $(1) | cat; \
	cmake --version; \
	touch $(2);
endef

# Macro of Extracting Zephyr SDK
# $(1): SDK file
# $(2): SDK directory
define EXTRACT_SDK
	sh $(1) -- -d $(2); \
	touch $(2);
endef

# Respective Targets
################################################################
.PHONY: cmake
cmake:
	@ if [ ! -d $($@_DIR) ]; then \
		$(call MESSAGE,"Preparing $@"); \
		$(call EXTRACT_CMAKE,$(notdir $($@_FILE)),$($@_DIR)) \
	fi
	@ if ( find $($@_FILE) -newer $($@_DIR) | grep -q $($@_FILE) ); then \
		$(call MESSAGE,"Updating $@"); \
		rm -rf $($@_DIR); \
		$(call EXTRACT_CMAKE,$(notdir $($@_FILE)),$($@_DIR)) \
	fi

.PHONY: sdk
sdk:
	@ if [ ! -d $($@_DIR) ]; then \
		$(call MESSAGE,"Preparing $@"); \
		$(call EXTRACT_SDK,$($@_FILE),$($@_DIR)) \
	fi
	@ if ( find $($@_FILE) -newer $($@_DIR) | grep -q $($@_FILE) ); then \
		$(call MESSAGE,"Updating $@"); \
		rm -rf $($@_DIR); \
		$(call EXTRACT_SDK,$($@_FILE),$($@_DIR)) \
	fi

.PHONY: kernel
kernel:
	echo $($@_BUILD_DIR)
	@ if [ ! -d $($@_BUILD_DIR) ]; then mkdir -p $($@_BUILD_DIR); fi
	(source $($@_DIR)/zephyr-env.sh && cd $($@_BUILD_DIR) && \
	printenv ZEPHYR_BASE && \
	cmake -DBOARD=$(BOARD) $($@_DIR)/samples/repeater/ && \
	printenv ZEPHYR_BASE && \
	make \
	)
