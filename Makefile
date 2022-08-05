TGT = culltest.elf
OBJ = culltest.o romfont.o
CXX=kos-c++

.PHONY: all run clean
all: $(TGT)

include $(KOS_BASE)/Makefile.rules

%.elf: $(OBJ)
	kos-c++ -o $@ $^

clean:
	rm -f $(TGT) $(OBJ)

run: $(TGT)
	$(KOS_LOADER) $(TGT)

$(TGT): $(OBJ)
