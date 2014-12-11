prefix?=/usr/local
exec_prefix?=$(prefix)

QUERY_TYPE?=JOIN

LIBEXECS=list_hashes paths_of_hashes
SCRIPTS=content_diff expire_and_clean
DOCS=COPYRIGHT LICENSE README

CFLAGS=-Wall -g -I /usr/include/postgresql -I $(prefix)/include -D$(QUERY_TYPE) 
LDFLAGS=-L /usr/lib/postgresql -lpq -L $(exec_prefix)/lib -L . -L $(exec_prefix)/lib/verity

all: $(LIBS) $(LIBEXECS) index_paths list_paths

clean:
	rm -f $(LIBS) $(LIBEXECS) index_paths list_paths

lib%.so: %.c
	cc -c -fpic -shared -Wall -g -o $@ $<

index_paths: index_paths.c sha256_of_file.c
	cc $(HACKS) $(CFLAGS) $(LDFLAGS) -lfgetsnull -lhexbytes -lcrypto -o $@  $^

paths_of_hashes: paths_of_hashes.c
	cc $(HACKS) $(CFLAGS) $(LDFLAGS) -o $@  $<

install:
	$(foreach prog, $(LIBS), install -D -m 0644 $(prog) $(exec_prefix)/lib/verity/$(prog); )
	$(foreach prog, $(LIBEXECS), install -D -m 0755 $(prog) $(exec_prefix)/lib/verity/$(prog); )
	install -D -m 0755 index_paths $(exec_prefix)/bin/verity_index_paths
	install -D -m 0755 list_paths $(exec_prefix)/bin/verity_list_paths
	$(foreach prog, $(SCRIPTS), install -D -m 0755 $(prog) $(exec_prefix)/bin/verity_$(prog); )
	$(foreach prog, $(DOCS), install -D -m 0644 $(prog) $(prefix)/share/doc/verity/$(prog); )
	install -D -m 0644 schema.psql $(prefix)/share/verity/schema.psql
	install -D -m 0644 get_passphrase $(prefix)/share/verity/get_passphrase

uninstall:
	rm $(exec_prefix)/bin/verity_index_paths
	rm $(exec_prefix)/bin/verity_list_paths
	$(foreach prog, $(SCRIPTS), rm $(exec_prefix)/bin/verity_$(prog); )
	rm -rf $(exec_prefix)/lib/verity
	rm -rf $(prefix)/share/doc/verity
	rm -rf $(prefix)/share/verity
	
#Copyright 2014 Michael Redman
#IN GOD WE TRVST.
