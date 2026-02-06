include $(PLATFORM_PATH)/sai.mk
include $(PLATFORM_PATH)/clounix-modules.mk
include $(PLATFORM_PATH)/platform-modules-clounix.mk
include $(PLATFORM_PATH)/docker-syncd-clounix.mk
include $(PLATFORM_PATH)/docker-syncd-clounix-rpc.mk
include $(PLATFORM_PATH)/one-image.mk
include $(PLATFORM_PATH)/docker-ptf-clounix.mk

SONIC_ALL += $(SONIC_ONE_IMAGE) $(DOCKER_FPM)

# Inject clounix sai into syncd
$(SYNCD)_DEPENDS += $(CLOUNIX_SAI) $(CLOUNIX_SAI_DEV)
$(SYNCD)_UNINSTALLS += $(CLOUNIX_SAI_DEV)

# Force the target bootloader for clounix platforms to grub regardless of arch
override TARGET_BOOTLOADER = grub

# Runtime dependency on clounix sai is set only for syncd
$(SYNCD)_RDEPENDS += $(CLOUNIX_SAI)
