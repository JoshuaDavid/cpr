all: bin/analyze bin/n_by_n_compare bin/venn bin/dmat2coord bin/better_compare

bin/analyze: src/analyze.c src/bit_vector.c src/shame.c src/hash.c src/index_and_search.c src/commet_job.c src/filter_reads.c src/compare_sets.c
	gcc $+ -g -Iinclude -lm -o bin/analyze

bin/n_by_n_compare: src/n_by_n_compare.c src/bit_vector.c src/shame.c src/hash.c src/index_and_search.c src/commet_job.c src/filter_reads.c src/compare_sets.c
	gcc $+ -g -Iinclude -lm -o bin/n_by_n_compare

bin/venn: src/venn.c src/bit_vector.c src/shame.c src/hash.c src/index_and_search.c src/commet_job.c src/filter_reads.c src/compare_sets.c
	gcc $+ -g -Iinclude -lm -o bin/venn

bin/dmat2coord: src/dmat2coord.c src/bit_vector.c src/shame.c src/commet_job.c
	gcc $+ -g -Iinclude -lm -o bin/dmat2coord

bin/better_compare: src/better_compare.c src/bit_vector.c src/shame.c src/hash.c src/index_and_search.c src/commet_job.c src/filter_reads.c src/compare_sets.c src/easy_pthreads.c
	gcc $+ -g -Iinclude -lm -pthread -o bin/better_compare

clean:
	rm -rf bin/*
