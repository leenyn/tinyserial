OBJ := tinyserial
SOURCES := tinyserial.c

RM = rm -f

all: $(OBJ)

$(OBJ): $(SOURCES)
	$(CC) $(CFLAGS) $(LDFLAGS) $(SOURCES) -o $@  $(LIBS)

install:$(OBJ)
	install -d ${DESTDIR}/${BINDIR}
	install -m 0755 $^ ${DESTDIR}/${BINDIR}/

clean:
	$(RM) *~ *.o $(OBJ)

