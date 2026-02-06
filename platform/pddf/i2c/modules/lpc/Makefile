TARGET := pddf_lpc_module
obj-m := $(TARGET).o

ccflags-y := -I$(M)/modules/include
# Auto-detect architecture and define CONFIG_ARCH_PHYTIUM for arm64/aarch64 if not defined by kernel
# Use ARCH from kernel build system if available, otherwise detect from uname
ARCH_DETECT := $(if $(ARCH),$(ARCH),$(shell uname -m))
ifeq ($(filter arm64 aarch64,$(ARCH_DETECT)),$(ARCH_DETECT))
    # Define CONFIG_ARCH_PHYTIUM for arm64/aarch64 platforms (e.g., Phytium)
    # This will be used if kernel doesn't define it via autoconf.h
    ccflags-y += -DCONFIG_ARCH_PHYTIUM
endif
