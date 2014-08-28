all: bin/analyze bin/n_by_n_compare

bin/analyze: src/analyze.c
	gcc src/analyze.c -Iinclude -lm -o bin/analyze

bin/n_by_n_compare: src/n_by_n_compare.c
	gcc src/n_by_n_compare.c -Iinclude -lm -o bin/n_by_n_compare

clean:
	rm -rf bin/*
