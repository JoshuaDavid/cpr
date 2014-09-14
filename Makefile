all: bin/analyze bin/n_by_n_compare

bin/analyze: src/analyze.c
	gcc $+ -g -Iinclude -lm -o bin/analyze

bin/n_by_n_compare: src/n_by_n_compare.c
	gcc $+ -g -Iinclude -lm -o bin/n_by_n_compare

clean:
	rm -rf bin/*
