RGBLIGHT_ENABLE = no
#CONSOLE_ENABLE = yes
#KCR_ENABLE = yes
LAO_ENABLE = yes
#TMA_ENABLE = yes
#MTH_ENABLE = yes
KA_ENABLE = yes

SRC += source.c

ifeq ($(strip $(KCR_ENABLE)), yes)
    OPT_DEFS += -DKCR_ENABLE
endif
ifeq ($(strip $(LAO_ENABLE)), yes)
    OPT_DEFS += -DLAO_ENABLE
endif
ifeq ($(strip $(TMA_ENABLE)), yes)
    OPT_DEFS += -DTMA_ENABLE
endif
ifeq ($(strip $(MTH_ENABLE)), yes)
    OPT_DEFS += -DMTH_ENABLE
endif
ifeq ($(strip $(KA_ENABLE)), yes)
    OPT_DEFS += -DKA_ENABLE
    SRC += key_action.c
endif
