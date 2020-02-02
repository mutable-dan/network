CC=g++-8

INSTALL_DIR = .
INCLUDE_DIR = -I../../


EXECLI   = client
EXESRV   = server
SOURCEC  = client.cpp 
SOURCES  = server.cpp
LINKLIBS = -lgsock
LIBLOC   = -L../../

OBJSC     = $(SOURCEC:.cpp=.o) 
DEPSC     = $(SOURCEC:.cpp=.d) 
OBJSS     = $(SOURCES:.cpp=.o) 
DEPSS     = $(SOURCES:.cpp=.d) 

-include $(DEPS)

CFLAGSALL     = -std=c++17 -Wall -Wextra -Werror -Wshadow -march=native -fno-default-inline -fno-stack-protector -pthread -Wall -Werror -pedantic -Wextra -Weffc++ -Waddress -Warray-bounds -Wno-builtin-macro-redefined -Wundef
CFLAGSRELEASE = -O2 -DNDEBUG $(CFLAGSALL)
CFLAGSDEBUG   = -ggdb3 -DDEBUG $(CFLAGSALL)

.PHONY: release
release: CFLAGS = $(CFLAGSRELEASE)
release: all

.PHONY: debug
debug: CFLAGS = $(CFLAGSDEBUG)
debug: all


# compile and link

all : $(OBJSC) $(OBJSS)
	$(CC) -o $(EXECLI) $(OBJSC) $(LIBLOC) $(LINKLIBS)
	$(CC) -o $(EXESRV) $(OBJSS) $(LIBLOC) $(LINKLIBS)

%.o: %.cpp
	$(CC) $(CFLAGS) $(INCLUDE_DIR) -MMD -MP -c $< -o $@

install : all
	install -d $(INSTALL_DIR)
	install -m 750 $(EXECLI) $(INSTALL_DIR)
	install -m 750 $(EXESRV) $(INSTALL_DIR)

uninstall :
	/bin/rm -rf $(INSTALL_DIR)

clean :
	rm -f *.o $(EXECLI) *.d
	rm -f *.o $(EXESRV) *.d
