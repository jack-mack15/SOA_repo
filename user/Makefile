CFLAGS_WITH_INCLUDE = -I../include

REG = registerer.c
DEREG = deregisterer.c
VICTIM = user.c

all: reg dereg victim
reg: $(REG)
	gcc $(CFLAGS_WITH_INCLUDE) $(REG) -o reg

dereg: $(DEREG)
	gcc $(CFLAGS_WITH_INCLUDE) $(DEREG) -o dereg

victim: $(VICTIM)
	gcc $(VICTIM) -o victim

clean:
	rm -f reg dereg victim