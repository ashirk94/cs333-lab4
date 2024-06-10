CC = gcc
DEBUG = -g
PTHREAD = -pthread
CFLAGS = $(DEBUG) $(PTHREAD) -Wall -Wextra -Wshadow -Wunreachable-code \
-Wredundant-decls -Wmissing-declarations \
-Wold-style-definition -Wmissing-prototypes \
-Wdeclaration-after-statement -Wno-return-local-addr \
-Wunsafe-loop-optimizations -Wuninitialized -Werror \
-Wno-unused-parameter -Wno-string-compare -Wno-stringop-overflow \
-Wno-stringop-overread -Wno-stringop-truncation
LDFLAGS = -lcrypt
PROG1 = thread_hash
PROGS = $(PROG1)

all: $(PROGS)

$(PROG1): $(PROG1).o
	$(CC) $(CFLAGS) -o $@ $^ $(LDFLAGS)

$(PROG1).o: $(PROG1).c
	$(CC) $(CFLAGS) -c $<

clean cls:
	rm -f $(PROGS) *.o *~ \#* *.txt *.err *.cracked *.out *.plain *.log

tar:
	tar cvfa Lab4_${LOGNAME}.tar.gz *.[ch] [mM]akefile

git:
	git add .; \
	git commit -m "Makefile commit message"; \
	git push