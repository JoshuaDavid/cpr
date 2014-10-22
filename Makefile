all: bin/analyze bin/n_by_n_compare bin/venn bin/dmat2coord

bin/analyze: src/analyze.c src/bit_vector.c src/shame.c src/hash.c src/index_and_search.c src/commet_job.c src/filter_reads.c src/compare_sets.c
	gcc $+ -g -Iinclude -lm -o bin/analyze

bin/n_by_n_compare: src/n_by_n_compare.c src/bit_vector.c src/shame.c src/hash.c src/index_and_search.c src/commet_job.c src/filter_reads.c src/compare_sets.c
	gcc $+ -g -Iinclude -lm -o bin/n_by_n_compare

bin/venn: src/venn.c src/bit_vector.c src/shame.c src/hash.c src/index_and_search.c src/commet_job.c src/filter_reads.c src/compare_sets.c
	gcc $+ -g -Iinclude -lm -o bin/venn

bin/dmat2coord: src/dmat2coord.c src/bit_vector.c
	gcc $+ -g -Iinclude -lm -o bin/dmat2coord

clean:
	rm -rf bin/*
