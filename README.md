# I/O-Efficient Group Closeness Centrality Calculation

## Requirements

* The code is tested on a 64 bit Ubuntu Server 14.04.3 LTS.
* Tested on GCC 4.8.4 and LLVM 3.6.0
* Require the C++11 `pthread` support, `-pthread -std=c++11`.
* Require the SNAP library (will be automatically downloaded).
  * The used SNAP library is slightly different from the official version.
    If the script does not clone the code. Please clone the specified
    library from here: https://github.com/zzjjzzgggg/netsnap.git

## Usage

### A minimum running example

The folder `exam` contains a minimum running example.

```shell
    cd exam
    chmod u+x run_all.sh
    ./run_all.sh
```

The script will automatically download the required library, compile the
program, and run all the programs.


### Input graph format ###
Assume a graph is stored in the edgelist form, e.g.,
```
1  3
2  3
3  5
...
```

1. First, sort the edges by destination.
```shell
sort -n -k2,2 -k1,1 input_graph -o input_graph_sorted
```

2. Convert the graph into a binary edgelist format for the purpose of fast
   loading.
  * There is also a tool provided in the folder `netsnap/graphstat`.
  * Use the following command to convert a graph into binary edgelist
    format:
    ```shell
    graphstat -i:input_graph_sorted.gz -f:e
    ```

### Generate bit-strings

```
usage: genbits
    -job     [n]      0: split graph
		              1: gen bits
		              2: check graph (default: -1)
    -g       [str]    graph file name (default: )
    -bs      [n]      graph block size (default: 10000)
    -ps      [n]      page size (default: 1000)
    -H       [n]      max hops (default: 5)
    -N       [n]      n approx (default: 8)
    -m       [n]      bucket bits (default: 6)
    -r       [n]      more bits (default: 6)
```

#### usage:
1. Split the graph into blocks:
   ```shell
       genbits -g input_graph_sorted_be.gz -job 0
   ```

   * specify the block size by `-bs` parameter.
2. Generate bit-strings:
   ```shell
       genbits -g HEPTH.gz -job 1
   ```

   * `-ps` contronls page size, i.e., how many nodes are organized in one
     file.
   * `-H` maximum hops defined in the GC measure.
   * `-N`, `-m` and `-r` are parameters related to bit-strings.
3. Results will be saved in folders `blks` and `bits`.

### Run I/O-Efficient Greedy Algorithm

```
usage: iogreedy
    -job     [n]      0: run io-greedy
    -g       [str]    graph (default: )
    -B       [n]      budget (default: 500)
    -L       [flt]    lambda (default: 0.50)
    -H       [n]      max hops (default: 5)
```

#### usage:
1. Calculate reward gains for each node: `iogreedy -g HEPTH.gz -job 1`.
2. Run IO-efficient greedy algorithm: `iogreedy -g HEPTH.gz -job 2`.
   * `-B`, `-L` and `-H` are parameters related to the IO-efficienty greedy algorithm.
3. Results will be saved in the same fold of the graph.
