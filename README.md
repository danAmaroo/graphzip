# graphzip

Compresses the English Wikipedia link graph — 7.2M articles, 652M links — into
a bit-packed structure you can still traverse without decompressing it.

**C++20 · gap encoding + Elias gamma coding · BFS over compressed adjacency lists**

## Result

| | Bits/edge | Memory (1M-node subgraph) |
|---|---|---|
| Uncompressed (CSR) | 32.40 | 309.9 MB |
| **graphzip** | **20.46** | **195.7 MB** |

**1.58× smaller**, with every one of 80.2M edges round-tripping through the
compressed form exactly (verified node by node against the uncompressed graph).

## Demo

```
$ ./build/graphzip path "Grace Hopper" "Kevin Bacon"
Grace_Hopper -> American_Civil_War -> Cinema_of_the_United_States -> Kevin_Bacon  (3 hops, 4 ms)

$ ./build/graphzip verify
OK: 1000000 nodes, 80247404 edges round-tripped exactly
195.715 MB
20.4589 bits/edge
```

The graph is loaded once from a compressed on-disk format, and BFS walks it
by decoding each adjacency list on demand — no upfront decompression pass.

## How it works

Adjacency lists are sorted, so neighbours are stored as gaps between
consecutive targets instead of absolute IDs, and each gap is written with
Elias gamma coding — a self-delimiting variable-length code, so small gaps
cost a handful of bits instead of a fixed 32. A per-node bit-offset index
keeps every node's list randomly accessible, which is what makes this a
graph you can query rather than an archive you have to unpack first.

## Build & run

```bash
cmake -B build -G Ninja -DCMAKE_BUILD_TYPE=RelWithDebInfo
cmake --build build
./build/tests/tests
```

Requires a C++20 compiler; Catch2 is fetched at configure time. The full
pipeline (parsing the raw Wikipedia SQL dumps into this graph format) is in
`src/main.cpp`.
