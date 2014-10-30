#OVERVIEW

Allows for quick and easy comparisons between groups of very large FASTA files. The overview can then be used to determine whether or not the groups are significantly different. Additionally, this program will be able to say which reads occurred in one group of files, and not the other (though I'm still in the process of implementing that feature).

For best results, it is ideal to use at least four files per set. Splitting each set into more files will improve the accuracy of the results at the cost of increased runtime.

#How it Works

##Comparing two files

Looks at all reads in both files. Reads are considered to be shared if they share a kmer of at least `kmer_length`. Similarity between two files can be calculated by counting the number of shared reads between the two files, then dividing by the number of comparisons made. The following diagram should clarify this:

If we have two files, A and B, with 4 and 7 reads respectively

   | B1 | B2 | B3 | B4 | B5 | B6 | B7 |
---|:--:|:--:|:--:|:--:|:--:|:--:|:--:|
A1 | +  |    |    |    |    |    | +  |
A2 |    |    | +  |    | +  |    |    |
A3 |    | +  |    |    | +  |    | +  |
A4 |    |    |    |    |    |    |    |

In this case, there are 7 shared reads, out of 28 (4 x 7) comparisons. This gives a similarity of 25% between the files.

##Comparing two sets

Each file in the first set is compared against every other file in the other set. Again, the number of positives over the number of comparisons is the similarity fraction.

## Visualization

Currently, visualization is used by converting the similarity matrix to a distance matrix (distance = -log(similarity), since two files with similarity == 0 could be considered infinitely distant, and two files with similarity = 100% would have a distance of 0).

This distance matrix is converted to a 2d graph through a stress minimization algorithm.

#Usage

Make a file in the following format which contains lists of fasta files, which define your sets

###job.txt

    SET1: s1a.fa; s1b.fa; s1c.fa
    SET2: s2a.fa; s2b.fa; s2c.fa
    SET3: s3a.fa; s3b.fa; s3c.fa

```
$ make
$ /path/to/cpr/bin/n_by_n_compare [-o output/directory] [-k kmer_length] [-p (0 / 1)] job.txt
$ /path/to/cpr/bin/analyze [-o output/directory] job.txt > matrix.txt
$ /path/to/cpr/bin/dmat2coord matrix.txt > coords.txt
```

Then plot coord.txt using your favorite graphing software (I will eventually make it so that it can be plotted using gnuplot).
