# I/O-Efficient Group Closeness Centrality Calculation

## Requirements

* The code is tested on a 64 bit Ubuntu Server 14.04.3 LTS.
* Require the C++11 support.
* Require the SNAP library.
  * The used SNAP library is slightly different from the official version.
    Please check out the specified library from here:
    https://github.com/zzjjzzgggg/netsnap.git

## Usage

### Generate bit-strings

```
usage: genbits
    -job     [n]      0: split graph
		              1: gen bits
		              2: validate (default: -1)
    -g       [str]    graph file name (default: )
    -bs      [n]      graph block size (default: 10000)
    -ps      [n]      page size (default: 1000)
    -H       [n]      max hops (default: 5)
    -N       [n]      n approx (default: 8)
    -m       [n]      bucket bits (default: 6)
    -r       [n]      more bits (default: 6)
```

#### Example:
1. Assume a graph is stored in the edgelist form, e.g.,
```
1  3
2  3
3  5
...
```
2. First, sort the edges by source vertices, e.g., `sort -n HEPTH -o HEPTH_sorted`.
3. Split the graph into blocks: `genbits -g HEPTH.gz -job 0`.
   * GZip format of the graph file is supported.
   * specify the block size by `-bs` parameter.
4. Generate bit-strings: `genbits -g HEPTH.gz -job 1`.
   * `-ps` contronls page size, i.e., how many nodes are organized in one file.
   * `-H` maximum hops defined in the GC measure.
   * `-N`, `-m` and `-r` are parameters related to bit-strings.
5. Results will be saved in folders `blks` and `bits`.

### Run I/O-Efficient Greedy Algorithm

```
usage: iogreedy
    -job     [n]      0: evaluate approximation performance
                      1: node reward gains
		              2: io greedy
		              3: multi pass (default: -1)
    -g       [str]    graph (default: )
    -B       [n]      budget (default: 500)
    -L       [flt]    lambda (default: 0.50)
    -H       [n]      max hops (default: 5)
    -S       [n]      group size (default: 10)
    -R       [n]      num repeat (default: 100)
```

#### Example:
1. Calculate reward gains for each node: `iogreedy -g HEPTH.gz -job 1`.
2. Run IO-efficient greedy algorithm: `iogreedy -g HEPTH.gz -job 2`.
   * `-B`, `-L` and `-H` are parameters related to the algorithm.
3. Results will be saved in the same fold of the graph.