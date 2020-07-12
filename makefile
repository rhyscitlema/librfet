# Makefile created by Rhyscitlema
# Explanation of file structure available at:
# http://rhyscitlema.com/applications/makefile.html

OUTPUT_FILE = librfet.a

OBJECT_FILES = rfet.o \
               operations.o \
               structures.o \
               expression.o \
               component.o

LIBALGO = ../algorithms
LIB_STD = ../lib_std
LIBRFET = .

#-------------------------------------------------

# C compiler
CC = gcc

# archiver
AR = ar

# compiler flags
CC_FLAGS = -I$(LIBALGO) \
           -I$(LIB_STD) \
           -I$(LIBRFET) \
           -Wall \
           -std=c99 \
           -pedantic \
           $(CFLAGS)

# archiver flags
AR_FLAGS = -crs #$(ARFLAGS)

#-------------------------------------------------

make: $(OUTPUT_FILE)

$(OUTPUT_FILE): $(OBJECT_FILES)
	$(AR) $(AR_FLAGS) $(OUTPUT_FILE) $(OBJECT_FILES)

# remove all created files
clean:
	$(RM) *.o *.a

#-------------------------------------------------

INCLUDE_FILES = $(LIBALGO)/*.h \
                $(LIB_STD)/*.h \
                $(LIBRFET)/*.h

# compile .c files to .o files
%.o: %.c $(INCLUDE_FILES)
	$(CC) $(CC_FLAGS) -c -o $@ $<

