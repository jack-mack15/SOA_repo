CFLAGS_WITH_INCLUDE = -I../include

INTER = interface.c
VICTIM = user.c

all: inter victim
inter: $(INTER)
	gcc $(CFLAGS_WITH_INCLUDE) $(INTER) -o inter

victim: $(VICTIM)
	gcc $(VICTIM) -o victim

clean:
	rm -f inter victim