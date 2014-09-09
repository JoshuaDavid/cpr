all: bin/analyze bin/n_by_n_compare bin/venn

bin/analyze: src/analyze.c
	gcc src/analyze.c -g -Iinclude -lm -o bin/analyze

bin/n_by_n_compare: src/n_by_n_compare.c
	gcc src/n_by_n_compare.c -g -Iinclude -lm -o bin/n_by_n_compare

bin/venn: src/venn.c
	gcc src/venn.c -g -Iinclude -lm -o bin/venn

clean:
	rm -rf bin/*
