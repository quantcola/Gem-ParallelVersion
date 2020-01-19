INCDIR = $(GARFIELD_HOME)/Include
LIBDIR = $(GARFIELD_HOME)/Library

# Compiler flags
CFLAGS = -Wall -Wextra -Wno-long-long \
	`root-config --cflags` \
	-O3 -fno-common -c \
	-I$(INCDIR)

LDFLAGS = -L$(LIBDIR) -lGarfield
LDFLAGS += `root-config --glibs` -lGeom -lgfortran -lm -lstdc++fs 

CFLAGS2= -lstdc++fs -pthread
	

gem: gem.C control.C
	$(CXX)  gem.C $(CFLAGS) -lstdc++fs
	$(CXX) control.C $(CFLAGS2) -o run 
	$(CXX) `root-config --cflags` -o gem gem.o $(LDFLAGS)
	rm gem.o

