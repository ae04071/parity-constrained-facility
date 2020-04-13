# Data Format

n, m: # of Client, # of Facility

p_i (i = 1 ... m): parity constraint
 - 0: unconstraint
 - 1: odd
 - 2: even
 
o_i (i = 1 ... m): Opening Cost (o_i is real number)

a_(i, j) (i = 1 ... m, j = 1 ... n): distance between facility i and client j (a_(i,j) is real number)


## Directory Format
[NR-Facility]-[NR-Client]/[unconstrained ratio][odd ratio][even ratio]/[input or output]/.plc

For example, if you want to get a data whose:
 - num of F is 500, # of D is 500,
 - unconstraind, odd, and even ratio is 1 : 1 : 2
 - input file
Then, the path is 500-500/112/input/.plc
