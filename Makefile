CC = gcc
DEBUG = -g
CFLAGS = $(DEBUG) -Wall -Wextra -Wshadow -Wunreachable-code \
-Wredundant-decls -Wmissing-declarations \
-Wold-style-definition -Wmissing-prototypes \
-Wdeclaration-after-statement -Wno-return-local-addr \
-Wunsafe-loop-optimizations -Wuninitialized -Werror \
-Wno-unused-parameter
PROG1 = thread_hash
PROGS = $(PROG1)

all: $(PROGS)

$(PROG1): $(PROG1).o
	$(CC) $(CFLAGS) -o $@ $^

$(PROG1).o: $(PROG1).c
	$(CC) $(CFLAGS) -c $<

clean cls:
	rm -f $(PROGS) *.o *~ \#*

tar:
	tar cvfa Lab4_${LOGNAME}.tar.gz *.[ch] [mM]akefile

git:
	git add .; \
	git commit -m "Makefile commit message"; \
	git push