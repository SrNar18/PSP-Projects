TARGET = narcade
OBJS = src/psp_main.o src/game.o src/assets.o
INCDIR = src
CFLAGS = -O2 -G0 -Wall -Wextra -std=gnu99
CXXFLAGS = $(CFLAGS) -fno-exceptions -fno-rtti
ASFLAGS = $(CFLAGS)
BUILD_PRX = 1
PSP_FW_VERSION = 600
LIBS = -lpspaudiolib -lpspaudio -lpsppower -lm
EXTRA_TARGETS = EBOOT.PBP
PSP_EBOOT_TITLE = Narcade
PSP_EBOOT_ICON = assets/ICON0.png
PSP_LARGE_MEMORY = 0
PSPSDK = $(shell psp-config --pspsdk-path)
include $(PSPSDK)/lib/build.mak
