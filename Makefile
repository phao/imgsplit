CC := clang

CC_FLAGS := -std=c99 -Wall -pedantic -Wextra -Warray-bounds \
	-Wunreachable-code -pipe $(shell sdl2-config --cflags) -g3

CC_LIBS := $(shell sdl2-config --libs) -lSDL2_image

C_FILES := $(wildcard *.c)
OBJ_FILES := $(patsubst %.c,%.o,$(C_FILES))
DEP_FILES := $(patsubst %.c,%.d,$(C_FILES))

imgsplit: $(OBJ_FILES)
	$(CC) $(OBJ_FILES) -o imgsplit $(CC_LIBS)

$(DEP_FILES): %.d: %.c
	$(CC) $(CC_FLAGS) -MM $< > $@

$(OBJ_FILES): %.o: %.c
	$(CC) $(CC_FLAGS) -c $< -o $@

-include $(DEP_FILES)

.PHONY: clear

clear:
	rm -f $(OBJ_FILES) $(DEP_FILES) imgsplit
