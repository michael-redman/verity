prefix?=/usr/local
exec_prefix?=$(prefix)

QUERY_TYPE?=JOIN

LIBEXECS=paths
PROGS=list hashes update sort
SCRIPTS=diff clean
DOCS=COPYRIGHT LICENSE README

CFLAGS=-Wall -g -fstack-protector -O2 -I /usr/include/postgresql -I $(prefix)/include -D$(QUERY_TYPE) 
LDFLAGS=-L /usr/lib/postgresql -lpq -L $(exec_prefix)/lib -L . -L $(exec_prefix)/lib/verity

all: $(LIBS) $(LIBEXECS) $(PROGS)

.PHONY: clean
clean:
	rm -f $(LIBS) $(LIBEXECS) $(PROGS)

hashes: hashes.c
	cc $(CFLAGS) $(LDFLAGS) -lfgetsnull -o $@ $<

update: update.c sha256_of_file.c
	cc $(HACKS) $(CFLAGS) $(LDFLAGS) -lfgetsnull -lhexbytes -lcrypto -o $@  $^

sort: sort.c vector.c
	cc -Wall -g -fstack-protector -o $@ $^ -lfgetsnull

install:
	$(foreach prog, $(LIBEXECS), install -D -m 0755 $(prog) $(exec_prefix)/lib/verity/$(prog); )
	$(foreach prog, $(PROGS), install -D -m 0755 $(prog) $(exec_prefix)/bin/verity_$(prog); )
	$(foreach prog, $(SCRIPTS), install -D -m 0755 $(prog) $(exec_prefix)/bin/verity_$(prog); )
	$(foreach prog, $(DOCS), install -D -m 0644 $(prog) $(prefix)/share/doc/verity/$(prog); )
	install -D -m 0644 schema.psql $(prefix)/share/verity/schema.psql
	install -D -m 0644 verity.7 $(prefix)/share/man/man7/verity.7
	install -D -m 0644 verity_list.1 $(prefix)/share/man/man1/verity_list.1
	install -D -m 0644 verity_update.1 $(prefix)/share/man/man1/verity_update.1
	install -D -m 0644 verity_hashes.1 $(prefix)/share/man/man1/verity_hashes.1
	install -D -m 0644 verity_diff.1 $(prefix)/share/man/man1/verity_diff.1
	install -D -m 0644 verity_clean.1 $(prefix)/share/man/man1/verity_clean.1

uninstall:
	$(foreach prog, $(PROGS) $(SCRIPTS), rm $(exec_prefix)/bin/verity_$(prog); )
	rm -rf $(prefix)/share/doc/verity
	rm -rf $(prefix)/share/verity
	rm -rf $(exec_prefix)/lib/verity
	cd /usr/local/share/man/man1 && rm verity_list.1 verity_update.1 verity_hashes.1 verity_diff.1 verity_clean.1
	rm /usr/local/man/man7/verity.7
	
#IN GOD WE TRVST.
