# graphzip

Compressing the Wikipedia link graph without giving up traversal.

## Graph

Extracted from the English Wikipedia database dumps (~50 GB across three tables):
7,219,612 articles and 652,985,740 links. Measurements below use a
1,000,000-node subgraph with 80,247,404 edges (average out-degree 80.2).

## Results

| Representation | Bits/edge | Memory | BFS latency |
|---|---|---|---|
| Uncompressed CSR | 32.40 | 309.9 MB | 85.8 ms |

BFS latency is the mean over 10 fixed random source/target pairs (seeded, so the
same pairs are used for every representation). All ten resolved in 3–4 hops;
individual times ranged 20–182 ms, dominated by frontier size rather than by
neighbour access. Load time from disk: 3.6 s.

/// add later
# how it works

# building it