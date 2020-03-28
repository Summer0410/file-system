#
# Copyright 2018 Jonathan Anderson
#
# Licensed under the Apache License, Version 2.0 (the "License");
# you may not use this file except in compliance with the License.
# You may obtain a copy of the License at
#
#     http://www.apache.org/licenses/LICENSE-2.0
#
# Unless required by applicable law or agreed to in writing, software
# distributed under the License is distributed on an "AS IS" BASIS,
# WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
# See the License for the specific language governing permissions and
# limitations under the License.
#

CFLAGS=	-g -Wall `pkg-config --cflags fuse` \
	#-D USE_EXAMPLE 
	-fsanitize=address -fno-omit-frame-pointer

LDFLAGS=`pkg-config --libs fuse` -fsanitize=address

OBJS=	\
	assign4.o \
	example.o \
	fuse.o \

all: fuse
fuse: ${OBJS}
	${CC} ${OBJS} ${LDFLAGS} -o fuse

# Express header dependencies: recompile these object files if header changes
fuse.o: assign4.h
assign4.o: assign4.h

clean:
	rm -f fuse *.o