CC = gcc
CFLAGS =  -Wall -fPIC
# LDFLAGS = -fPIC

.PHONY: build
build: libscheduler.so

libscheduler.so: so_scheduler.o gfg_pq.o thread_stuff.o
	$(CC) $(LDFLAGS) -shared -o $@ $^

so_scheduler.o: so_scheduler.c 
	$(CC) $(CFLAGS) -o $@ -c $<

gfg_pq.o: gfg_pq.c 
	$(CC) $(CFLAGS) -o $@ -c $<

thread_stuff.o: thread_stuff.c 
	$(CC) $(CFLAGS) -o $@ -c $<

.PHONY: clean
clean:
	-rm -f so_scheduler.o libscheduler.so gfg_pq.o thread_stuff.o