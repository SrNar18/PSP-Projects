TARGET = narcade
OBJS = src/psp_main.o src/game.o src/render3d.o src/assets.o src/textures3d.o src/icon0.o
INCDIR = src
CFLAGS = -O2 -G0 -Wall -Wextra -std=gnu99 -DNARCADE_3D
CXXFLAGS = $(CFLAGS) -fno-exceptions -fno-rtti
ASFLAGS = $(CFLAGS)
BUILD_PRX = 1
PSP_FW_VERSION = 600
LIBS = -lpspgum -lpspgu -lpsputility -lpspaudiolib -lpspaudio -lpsppower -lm
EXTRA_TARGETS = EBOOT.PBP
PSP_EBOOT_TITLE = Narcade 3D
PSP_EBOOT_ICON = assets/ICON0.png
PSP_EBOOT_PIC1 = assets/PIC1.png
PSP_EBOOT_SND0 = assets/SND0.AT3
PSP_LARGE_MEMORY = 0
PSPSDK = $(shell psp-config --pspsdk-path)
include $(PSPSDK)/lib/build.mak
