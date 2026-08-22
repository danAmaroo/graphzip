# graphzip

A compressed graph engine for Wikipedia's link structure

## Milestones

### M1 — Bit stream 
BitWriter and BitReader, MSB-first, spilling across byte boundaries correctly. Round-trip test over 10,000 random (value, width) pairs including widths 1, 7, 8, 9, 32, 64.


### M2 — Codes 
Elias gamma on top of the bit stream: write length-in-unary then the value. Delta if you want it. Round-trip test over values from 1 to 2³².


### M3 — Data 
Download pagelinks and page dumps, filter to namespace 0, intern titles to dense integer IDs, emit a sorted edge list.
Also extract a 100K-node subset here — everything downstream gets developed against it, and only final numbers run on the full graph.


### M4 — Baseline 
Uncompressed CSR: one array of concatenated neighbours, one of offsets. BFS on top.


### M5 — Gap encoding
Store differences between consecutive neighbours rather than absolute IDs, gamma-coded. Per-node offsets into the bit stream so you can seek to any node.


### M6 — Interval encoding 
Runs of consecutive IDs stored as start plus length. Requires a scheme for signalling which representation a list uses.


### M7 — Reference encoding 
Encode a node's list as a delta against a similar earlier node within a backward window. Copy-list bitmap plus extras. Bounded reference chains.


### M8 — Node ordering 
Reorder nodes lexicographically by title and re-run the encoder. Compare against the original ordering.


### M9 — Traversal and benchmarks 
BFS decoding neighbours on demand, never materialising the graph. Naive versus compressed on memory and query latency.
