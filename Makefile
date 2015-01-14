prefix?=/usr/local
exec_prefix?=$(prefix)

QUERY_TYPE?=JOIN

PROGS=update list
LIBEXECS=paths_of_hashes
SCRIPTS=diff clean
DOCS=COPYRIGHT LICENSE README

CFLAGS=-Wall -g -fstack-protector -I /usr/include/postgresql -I $(prefix)/include -D$(QUERY_TYPE) 
LDFLAGS=-L /usr/lib/postgresql -lpq -L $(exec_prefix)/lib -L . -L $(exec_prefix)/lib/verity

all: $(LIBS) $(LIBEXECS) $(PROGS)

.PHONY: clean
clean:
	rm -f $(LIBS) $(LIBEXECS) $(PROGS)

lib%.so: %.c
	cc -c -fpic -shared -Wall -g -o $@ $<

update: update.c sha256_of_file.c
	cc $(HACKS) $(CFLAGS) $(LDFLAGS) -lfgetsnull -lhexbytes -lcrypto -o $@  $^

paths_of_hashes: paths_of_hashes.c
	cc $(HACKS) $(CFLAGS) $(LDFLAGS) -o $@  $<

install:
	$(foreach prog, $(LIBEXECS), install -D -m 0755 $(prog) $(exec_prefix)/lib/verity/$(prog); )
	$(foreach prog, $(PROGS), install -D -m 0755 $(prog) $(exec_prefix)/bin/verity_$(prog); )
	$(foreach prog, $(SCRIPTS), install -D -m 0755 $(prog) $(exec_prefix)/bin/verity_$(prog); )
	$(foreach prog, $(DOCS), install -D -m 0644 $(prog) $(prefix)/share/doc/verity/$(prog); )
	install -D -m 0644 schema.psql $(prefix)/share/verity/schema.psql

uninstall:
	$(foreach prog, $(PROGS) $(SCRIPTS), rm $(exec_prefix)/bin/verity_$(prog); )
	rm -rf $(exec_prefix)/lib/verity
	rm -rf $(prefix)/share/doc/verity
	rm -rf $(prefix)/share/verity
	
#Copyright 2014 Michael Redman
#IN GOD WE TRVST.
