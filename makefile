CC=g++-8

LIB_INSTALL_DIR = /usr/lib/
INCLUDE_DIR = -I. 
LINK_LIBS := -lpthread 

LIB = libgsock.so
SOURCE = sockets.cpp 

OBJS = $(SOURCE:.cpp=.o) 
DEPS = $(SOURCE:.cpp=.d) 

-include $(DEPS)

VERBOSE = -ggdb3 -DDEBUG 
DEBUG   = -ggdb3 -DDEBUG
PROD    = -DNDEBUG 
RELEASE = -O2 -DNDEBUG 

FLAGS   = -march=native -mtune=native -fno-default-inline -fno-stack-protector -pthread -Wall -Werror -pedantic -Wextra -Weffc++ -Waddress -Warray-bounds -Wno-builtin-macro-redefined -Wundef
FLAGS  += -std=c++17

FLAGSVERBOSE  = $(FLAGS)
FLAGSVERBOSE += $(VERBOSE)
FLAGSDEBUG    = $(FLAGS)
FLAGSDEBUG   += $(DEBUG)
FLAGSPROD     = $(FLAGS)
FLAGSPROD    += $(PROD)
FLAGSRELEASE  = $(FLAGS)
FLAGSRELEASE += $(RELEASE)


.PHONY: prod
prod: CFLAGS = $(FLAGSPROD) -c -fPIC
prod: all
prod: install

.PHONY: release
release: CFLAGS = $(FLAGSRELEASE) -c -fPIC
release: all
#release: install

.PHONY: debug
debug: CFLAGS = $(FLAGSDEBUG) -c -fPIC
debug: all
#debug: install

.PHONY: verbose 
verbose: CFLAGS = $(FLAGSVERBOSE) -c -fPIC
verbose: all
#verbose: install

#all : $(OBJS) $(DEPS) 
all : $(OBJS)
	$(CC) -shared -o $(LIB) $(OBJS) $(LINK_LIBS)
	
%.o: %.cpp
	$(CC) $(CFLAGS) -MMD -MP -c $< -o $@

#%.d: %.cpp
#	$(CC) $(CFLAGS) -MM -MT $< -o $@
# -MF  write the generated dependency rule to a file
# -MG  assume missing headers will be generated and don't stop with an error
# -MM  generate dependency rule for prerequisite, skipping system headers
# -MP  add phony target for each header to prevent errors when header is missing
# -MT  add a target to the generated dependency
	

clean :
	rm -f *.o *.so *.d

install : all
	install -d $(LIB_INSTALL_DIR)
	install -m 755 $(LIB) $(LIB_INSTALL_DIR)

uninstall :
	/bin/rm -rf $(LIB_INSTALL_DIR)

