prefix?=/usr/local
exec_prefix?=$(prefix)

QUERY_TYPE?=JOIN

PROGS=list hashes update
SCRIPTS=diff clean
DOCS=COPYRIGHT LICENSE README

CFLAGS=-Wall -g -fstack-protector -O2 -I /usr/include/postgresql -I $(prefix)/include -D$(QUERY_TYPE) 
LDFLAGS=-L /usr/lib/postgresql -lpq -L $(exec_prefix)/lib -L . -L $(exec_prefix)/lib/verity

all: $(LIBS) $(LIBEXECS) $(PROGS) paths

.PHONY: clean
clean:
	rm -f $(LIBS) $(LIBEXECS) $(PROGS) paths

hashes: hashes.c
	cc $(CFLAGS) $(LDFLAGS) -lfgetsnull -o $@ $<

update: update.c sha256_of_file.c
	cc $(HACKS) $(CFLAGS) $(LDFLAGS) -lfgetsnull -lhexbytes -lcrypto -o $@  $^

paths: paths.c
	cc -Wall -g -fstack-protector -O2 -o $@ $<

install:
	$(foreach prog, $(PROGS), install -D -m 0755 $(prog) $(exec_prefix)/bin/verity_$(prog); )
	$(foreach prog, $(SCRIPTS), install -D -m 0755 $(prog) $(exec_prefix)/bin/verity_$(prog); )
	$(foreach prog, $(DOCS), install -D -m 0644 $(prog) $(prefix)/share/doc/verity/$(prog); )
	install -D -m 0755 paths $(exec_prefix)/lib/verity/paths
	install -D -m 0644 schema.psql $(prefix)/share/verity/schema.psql

uninstall:
	$(foreach prog, $(PROGS) $(SCRIPTS), rm $(exec_prefix)/bin/verity_$(prog); )
	rm -rf $(prefix)/share/doc/verity
	rm -rf $(prefix)/share/verity
	rm -rf $(exec_prefix)/lib/verity
	
#IN GOD WE TRVST.
