all: bin/analyze bin/n_by_n_compare bin/venn bin/dmat2coord bin/deeper_analysis bin/markov

bin/analyze: src/analyze.c src/bit_vector.c src/shame.c src/hash.c src/index_and_search.c src/commet_job.c src/filter_reads.c src/compare_sets.c
	gcc $+ -g -Iinclude -lm -o bin/analyze

bin/n_by_n_compare: src/n_by_n_compare.c src/bit_vector.c src/shame.c src/hash.c src/index_and_search.c src/commet_job.c src/filter_reads.c src/compare_sets.c
	gcc $+ -g -Iinclude -lm -o bin/n_by_n_compare

bin/venn: src/venn.c src/bit_vector.c src/shame.c src/hash.c src/index_and_search.c src/commet_job.c src/filter_reads.c src/compare_sets.c
	gcc $+ -g -Iinclude -lm -o bin/venn

bin/dmat2coord: src/dmat2coord.c src/bit_vector.c src/shame.c src/commet_job.c
	gcc $+ -g -Iinclude -lm -o bin/dmat2coord

bin/better_compare: src/better_compare.c src/bit_vector.c src/shame.c src/hash.c src/index_and_search.c src/commet_job.c src/filter_reads.c src/compare_sets.c src/easy_pthreads.c src/extract_reads.c
	gcc $+ -g -Iinclude -lm -pthread -o bin/better_compare

bin/deeper_analysis: src/deeper_analysis.c src/bit_vector.c src/shame.c src/hash.c src/index_and_search.c src/commet_job.c src/filter_reads.c src/compare_sets.c src/easy_pthreads.c src/extract_reads.c
	gcc $+ -g -Iinclude -lm -pthread -o bin/deeper_analysis

bin/markov:
	cp src/markov.pl bin/markov
	chmod a+x bin/markov

clean:
	rm -rf bin/*
