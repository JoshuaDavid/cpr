#!/usr/bin/perl
use strict;
use warnings;
use threads;
use threads::shared;

my ($jobfile) = @ARGV;
mkdir "output";
#get_distance_matrix($files, 6);

# Read in sets
my $sets = {};
open(FILE, "<$jobfile");
my $allfiles = [];
while(my $line = <FILE>) {
    chomp($line);
    my ($set_name, $filenames_raw) = split(m/\s*:\s*/, $line);
    $set_name =~ s/^\s+|\s+$//g; # trim
    my $filenames = [];
    @$filenames = split(m/\s*\;\s*/, $filenames_raw);
    push(@$allfiles, @$filenames);
    $sets->{$set_name} = $filenames;
}
close(FILE);

my $distances = get_distance_matrix($allfiles, 8);
my $coords = get_coords_from_distances($distances);
aggregate_outputs($sets);
print_coords_to_file($coords);
plot_coords();

sub aggregate_outputs {
    my ($sets) = @_;
    foreach my $a (keys %$sets) {
        foreach my $b (keys %$sets) {
            my $cmd = "cat output/$a"."_$a*_not_in_$b"."_$b*.fasta " .
                      "> output/set_$a" . "_not_in_set_$b.fasta\n";
            system($cmd);
            $cmd = "rm output/$a"."_$a*_not_in_$b"."_$b*.fasta\n";
            system($cmd);
        }
    }
}

# Output to coordinates file
sub print_coords_to_file {
    my ($coords) = @_;
    open(COORDS, ">output/coords.txt");
    foreach my $a (keys %$coords) {
        my $label = $a;
        $label =~ s/^\s+|\s+$//;
        $label =~ s/\W.*$//;
        print COORDS "$label $coords->{$a}->{'x'}  $coords->{$a}->{'y'}\n";
    }
    close(COORDS);
}

sub plot_coords {
    open(GNUPLOT, "|gnuplot");
    print GNUPLOT 'set term png' . "\n";
    print GNUPLOT 'set output "output/scatter_plot.png"' . "\n";
    print GNUPLOT 'set title "Scatter Plot"' . "\n";
    print GNUPLOT 'set grid' . "\n";
    print GNUPLOT 'set xlabel "X"' . "\n";
    print GNUPLOT 'set ylabel "Y"' . "\n";
    print GNUPLOT 'set size ratio 1' . "\n";
    print GNUPLOT 'set timestamp' . "\n";
    print GNUPLOT 'plot "output/coords.txt" using ($2):($3):1 with labels' . "\n";
    print GNUPLOT 'quit' . "\n";
    close(GNUPLOT);
}

sub pythagorean_distance {
    my ($a, $b) = @_;
    my $dx = $a->{"x"} - $b->{"x"};
    my $dy = $a->{"y"} - $b->{"y"};
    return sqrt($dx * $dx + $dy * $dy);
}

# Use a Hooke model to generate a 2d visualization of the data from a distance
# matrix.
sub get_coords_from_distances {
    my ($distances) = @_;
    my $nodes = {};
    my $conns = {};
    # Randomize the positions of each of the nodes.
    foreach my $a (keys %$distances) {
        $nodes->{$a} = {};
        $nodes->{$a}->{"x"} = rand();
        $nodes->{$a}->{"y"} = rand();
        # Strain starts at 0
        $nodes->{$a}->{"fx"} = 0;
        $nodes->{$a}->{"fy"} = 0;
        # Connections is a 2d array
    }

    my $total_strain = 0;
    for(my $i = 0; $i < 10000; $i++) {
        $total_strain = 0;
        foreach my $a (keys %$distances) {
            $nodes->{$a}->{"fx"} = 0;
            $nodes->{$a}->{"fy"} = 0;
            foreach my $b (keys %$distances) {
                my $model_d = $distances->{$a}->{$b};
                my $curr_d  = pythagorean_distance($nodes->{$a}, $nodes->{$b});
                my $dx = $nodes->{$b}->{"x"} - $nodes->{$a}->{"x"};
                my $dy = $nodes->{$b}->{"y"} - $nodes->{$a}->{"y"};
                # If current distance < model distance, force is positive, and
                # pushes the two nodes apart.
                my $f = $model_d - $curr_d;
                $total_strain += abs($f);
                my ($fx, $fy) = (0, 0);
                if($curr_d != 0) {
                    $fx = 0.01 * $f * $dx / $curr_d;
                    $fy = 0.01 * $f * $dy / $curr_d;
                }
                $nodes->{$a}->{"fx"} += $fx;
                $nodes->{$a}->{"fy"} += $fy;
            }
            $nodes->{$a}->{"x"} -= $nodes->{$a}->{"fx"};
            $nodes->{$a}->{"y"} -= $nodes->{$a}->{"fy"};
        }
    }
    return $nodes;
}

sub get_distance_matrix {
    my ($files, $depth) = @_;
    my $models = {};
    my $distances = {};
    my $threads = {};
    foreach my $name (@$files) {
        $threads->{$name} = threads->create('build_model', ($name, $depth));
    }
    foreach my $name (@$files) {
        $models->{$name} = $threads->{$name}->join();
    }
    $threads = {};
    foreach my $a (@$files) {
        my $title_a = $a;
        $title_a =~ s/\//_/g;
        $title_a =~ s/\.fa.*$//g;
        $distances->{$a} = {};
        $threads->{$a} = {};
        foreach my $b (@$files) {
            my $title_b = $b;
            $title_b =~ s/\//_/g;
            $title_b =~ s/\.fa.*$//g;
            $threads->{$a}->{$b} = threads->create('avg_entropy_diff', 
                ($a, $models->{$a}, $models->{$b}, $title_a, $title_b));
        }
        foreach my $b (@$files) {
            $distances->{$a}->{$b} = $threads->{$a}->{$b}->join();
            #print "\$distances->{\"$a\"}->{\"$b\"} = $distances->{$a}->{$b};\n"; 
        }
    }
    open(DISTANCES, ">output/distances.csv");
    print DISTANCES (" ," . join(",", @$files) . "\n");
    foreach my $a (@$files) {
        print DISTANCES "$a,";
        my $dists = [];
        foreach my $b (@$files) {
            push(@$dists, substr($distances->{$a}->{$b}, 0, 5));
        }
        print DISTANCES (join(",", @$dists) . "\n");
    }
    close(DISTANCES);
    return $distances;
}

sub build_model {
    my ($filename, $depth) = @_;
    my $freqs = get_markov_frequencies($filename, $depth);
    my $model = models_from_counts($freqs);
    return $model;
}

# Calculates the difference in entropy per base between two models in terms of
# how well they predict 
# according to a given markov model, and how much of that surprisingness
# is caused by each level of modeling.
sub avg_entropy_diff {
    my ($filename, $models_a, $models_b, $title_a, $title_b) = @_;
    open(IN, "<$filename");
    my $out_a = "output/$title_a" . "_not_in_" . $title_b . ".fasta";
    open(OUT_A, ">$out_a");
    my $m = (scalar @$models_a) - 1;
    my $entropy_a = 0;
    my $entropy_b = 0;
    my $num_bases = 0;
    my $linenum = 0;
    while(my $line = <IN>) {
        $line = uc $line;
        chomp($line);
        if($line =~ m/^[ACTG]+$/) {
            my $lineprob_markov_a = -get_lg_model_prob($models_a, $line, $m);
            my $lineprob_markov_b = -get_lg_model_prob($models_b, $line, $m);
            $entropy_a += $lineprob_markov_a;
            $entropy_b += $lineprob_markov_b;
            $num_bases += length($line);
            # If entropy of sequence under $model_a is at least 10 bits lower
            # than entropy under $model_b (i.e. 1000x more likely to occur
            # under $model_a) output the sequence.
            if($lineprob_markov_a <= $lineprob_markov_b - 10) {
                print OUT_A ">$filename" . "_line_" . "$linenum\n";
                print OUT_A "$line\n";
            }
        }
        $linenum++;
    }
    my $avg_entropy_model_a = $entropy_a / $num_bases;
    my $avg_entropy_model_b = $entropy_b / $num_bases;
    #print "model_a: $avg_entropy_model_a bits / base\n";
    #print "model_b: $avg_entropy_model_b bits / base\n";
    close(IN);
    close(OUT_A);
    #print "Wrote to $out_a\n";
    # Distance is the predictive disadvantage of model b as opposed to model a
    my $distance = $avg_entropy_model_b - $avg_entropy_model_a;
    return $distance;
}

sub print_hash {
    my ($hash) = @_;
    foreach my $key (sort(keys(%$hash))) {
        print "$key: $$hash{$key}\n";
    }
}

# Returns the sum of all values in a hash.
sub sum_hash {
    my ($hash) = @_;
    my $sum = 0;
    foreach my $val (values(%$hash)) {
        $sum += $val;
    }
    return $sum;
}


# Creates markov model of (seq => prob) from a hash of (seq => count)
sub model_from_counts {
    my ($counts) = @_;
    my $total_seqs = sum_hash($counts);
    my $model = {};
    while(my ($key, $value) = each(%$counts)) {
        $$model{$key} = $value / $total_seqs;
    }
    return $model;
}

# Creates an array of models with key length = (1 .. $m)
sub models_from_counts {
    my ($counts) = @_;
    my $models = [];
    my $m = length((%$counts)[0]);
    push(@$models, {});
    $models->[0]->{""} = 1;
    for my $i (1 .. ($m)) {
        my $model = model_from_counts(get_subseq_freqs($counts, $i));
        push(@$models, $model);
    }
    return $models;
}

# Given a hash full of the frequencies of sequences of length $n, returns a
# hash full for frequences of sequences of length $m where $m < $n.
sub get_subseq_freqs {
    my ($seq_freqs, $m) = @_;
    my $subseq_freqs = {};
    while(my ($seq, $count) = each(%$seq_freqs)) {
        my $n = length($seq);
        for my $i (0 .. ($n - $m)) {
            my $subseq = substr($seq, $i, $m);
            $$subseq_freqs{$subseq} += $$seq_freqs{$seq} / (1 + $n - $m);
        }
    }
    return $subseq_freqs;
}

# Extends a given markov model by 1 character.
# {A: 0.2, C: 0.3, G: 0.3, T: 0.2}
#   -> {AA: 0.04, AC: 0.06, ... CG: 0.09, CT: 0.06, ...}
sub extend_markov_model {
    my ($model, $times) = @_;
    if($times == 0) {
        return $model;
    } else {
        my $m = length((%$model)[0]);
        my $extended = {};
        if($m == 0) {
            $extended->{"A"} = 1 / 4;
            $extended->{"C"} = 1 / 4;
            $extended->{"G"} = 1 / 4;
            $extended->{"T"} = 1 / 4;
        } else {
            while(my ($seq, $prob) = each(%$model)) {
                my $prior = $model->{$seq};
                my $tail = substr($seq, 1);
                my $pNextA = $model->{$tail . "A"};
                my $pNextC = $model->{$tail . "C"};
                my $pNextG = $model->{$tail . "G"};
                my $pNextT = $model->{$tail . "T"};
                my $pNextN = $pNextA + $pNextC + $pNextG + $pNextT;
                $extended->{$seq . "A"} = $prior * $pNextA / $pNextN;
                $extended->{$seq . "C"} = $prior * $pNextC / $pNextN;
                $extended->{$seq . "G"} = $prior * $pNextG / $pNextN;
                $extended->{$seq . "T"} = $prior * $pNextT / $pNextN;
            }
        }
        return extend_markov_model($extended, $times - 1);
    }
}

# Log base 2
sub lg {
    my ($n) = @_;
    return 1.4426950408889634 * log($n);
}

# Get the lg(probability) of seeing a given sequence of length $n  with a given 
# markov model. $model_probs is a hash of the probabilities of each individual
# subsequence of length $m.
sub get_lg_model_prob {
    my ($models, $sequence, $m) = @_;
    my $n = length($sequence);
    if($m > $n) {
        print "Use a model with key length <= the length of the sequence you".
              "are looking for.\n";
        return ();
    } elsif(length($models) < $m) {
        print "Not enough models built to do that.\n";
        return ();
    } elsif($m == -1) {
        # Oddly enough, this does have a valid meaning as a degenerate case
        # $m is 1 + sequence length, and p(contains empty string) == 1
        return 1;
    } elsif($m == 0) {
        # No model: 1 / 4 chance of each base
        return length($sequence) * lg(1 / 4);
    } else {
        my $start = substr($sequence, 0, $m - 1);
        my $prior = $models->[$m - 1]->{substr($sequence, 0, $m - 1)};
        unless(defined($prior)) {
            $prior = exp(get_lg_model_prob($models, substr($sequence, 0, $m - 1), 1) * log(2));
        }
        my $lprior = log($prior);
        my $posterior = $lprior;
        for my $i (0 .. ($n - $m)) {
            $lprior = $posterior;
            my $start = substr($sequence, $i, $m - 1);
            my $full = substr($sequence, $i, $m);
            my $totalprob = $models->[$m - 1]->{$start};
            unless(defined($totalprob)) {
                #print "substr(\"$sequence\", $i, $m) == \"". substr($sequence, $i, $m) . "\"\n";
                $totalprob = exp(get_lg_model_prob($models, $start, 1) * log(2));
            }
            my $prob = $models->[$m]->{$full};
            unless(defined($prob)) {
                $prob = exp(get_lg_model_prob($models, $full, 1) * log(2));
                #print "substr(\"$sequence\", $i, $m) == \"". substr($sequence, $i, $m) . "\"\n";
            }
            my $condprob = $prob / $totalprob;
            $posterior += lg($condprob);
        }
        return $posterior;
    }
}

sub get_markov_frequencies {
    my ($filename, $seq_len) = @_;
    my $freqs = {};

    open FILE, "<$filename";

    while(my $line = <FILE>) {
        $line = uc $line;
        if($line =~ m/^[ACTG]+$/) {
            chomp($line);
            my $linelen = length $line;
            for my $i (0 .. ($linelen - $seq_len)) {
                my $subseq = substr($line, $i, $seq_len);
                $$freqs{$subseq} += 1;
            }
        } 
    }


    close FILE;
    return $freqs;
}
