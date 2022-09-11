################################################################################
#
#
#     Copyright (C) 2022 snickerbockers
#
#     This program is free software: you can redistribute it and/or modify
#     it under the terms of the GNU General Public License as published by
#     the Free Software Foundation, either version 3 of the License, or
#     (at your option) any later version.
#
#     This program is distributed in the hope that it will be useful,
#     but WITHOUT ANY WARRANTY; without even the implied warranty of
#     MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
#     GNU General Public License for more details.
#
#     You should have received a copy of the GNU General Public License
#     along with this program.  If not, see <http://www.gnu.org/licenses/>.
#
#
################################################################################

TGT = culltest.elf
OBJ = culltest.o romfont.o matrix.o
CXX=kos-c++

.PHONY: all run clean
all: $(TGT)

include $(KOS_BASE)/Makefile.rules

%.elf: $(OBJ)
	kos-c++ -o $@ $^ -lm

clean:
	rm -f $(TGT) $(OBJ)

run: $(TGT)
	$(KOS_LOADER) $(TGT)

$(TGT): $(OBJ)
