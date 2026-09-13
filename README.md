# graphzip

Compressing the Wikipedia link graph without giving up traversal.

A C++20 engine that stores the English Wikipedia article link graph in
gap-encoded, Elias-gamma-coded adjacency lists, and traverses it without
decompressing the whole structure.

## The graph

Extracted from the English Wikipedia database dumps — roughly 50 GB
uncompressed across three tables (`page`, `linktarget`, `pagelinks`):

- **7,219,612 articles** (namespace 0, excluding redirects)
- **652,985,740 links** between them

All measurements below use a **1,000,000-node subgraph** containing every link
between those nodes: **80,247,404 edges**, average out-degree 80.2. The subgraph
exists so that encoding passes take seconds rather than tens of minutes; the
full edge list is produced by the same pipeline.

## Results

| Representation | Bits/edge | Memory | BFS latency |
|---|---|---|---|
| CSR, direct span access | 32.40 | 309.9 MB | 85.8 ms |
| CSR, via copy interface | 32.40 | 309.9 MB | 120.7 ms |
| Gap-encoded, Elias gamma | **20.46** | **195.7 MB** | 1,520 ms |

**1.58× smaller. 12.6× slower to traverse.**

All 80,247,404 edges round-trip through the compressed representation exactly —
verified node by node against the uncompressed baseline.

BFS latency is the mean over ten fixed source/target pairs, chosen by a seeded
RNG so every representation answers identical queries. All ten resolved in 3–4
hops. The two baseline rows differ only in how neighbours are handed to the
caller: the first returns a `std::span` into existing memory, the second
materialises each list into a caller-supplied vector. The compressed
representation has no choice but to materialise, so the second row is the
honest comparison point; the first shows what the abstraction costs.

## How it works

### Uncompressed baseline

Compressed sparse row. One array holds every adjacency list concatenated end to
end; a second holds, for each node, the index where its list begins. Node *i*'s
neighbours are the range `[offsets[i], offsets[i+1])` — one lookup, then a
contiguous run of memory. The `n+1` sizing removes the special case for the
last node.

At 32 bits per target this is 32.40 bits per edge; the excess over 32 is the
offsets array amortised across 80 edges per node.

### Gap encoding

Adjacency lists are sorted, so consecutive targets can be stored as differences
rather than absolute values. A list of `[17, 23, 24, 89]` becomes `17` followed
by gaps. Because the lists are also deduplicated, every gap is at least 1, so
`gap - 1` is encoded instead — a run of consecutive targets then costs a single
bit each.

### Elias gamma

Gaps vary in magnitude, so they need variable-width encoding, which raises the
question of how the decoder knows where one value ends. A length prefix only
moves the problem. Gamma coding solves it with unary, which is self-delimiting:
write `⌊log₂x⌋` zeros, then `x` in binary. The terminating 1 of the unary run is
also the leading bit of the value, so nothing is wasted.

12 encodes in 7 bits rather than 32.

### Random access

A per-node index records the bit offset at which each list begins, so any node's
neighbours can be decoded without touching the rest of the structure. This costs
8 MB for a million nodes — about 0.8 bits per edge of the 20.46 total — and it
is what makes the compressed form a usable graph rather than an archive.

## Why traversal is 12.6× slower

The slowdown is larger than the bit savings alone would predict, and the causes
are in the decoder rather than in the encoding:

- `read_gamma` counts the unary prefix one bit at a time, a function call per
  bit, where the zero run could be found with a single count-leading-zeros
  instruction over a word.
- A `BitReader` is constructed on every `neighbours` call rather than being
  reused across a traversal.
- `read_bits` runs its byte-spanning loop even when the requested width sits
  entirely inside the current byte.

Beyond those, decoding is inherently sequential: the baseline's `memcpy` of a
contiguous 320-byte run is prefetched and pipelined by the CPU, while a chain of
gamma codes cannot be read ahead of itself.

Reducing this multiplier is more valuable than adding a further compression
stage, and is the next thing worth doing.

## Pipeline

```bash
# Extract the article set and assign dense IDs
gzip -dc data/enwiki-latest-page.sql.gz       | ./build/graphzip parse-page

# Resolve link targets against those articles
gzip -dc data/enwiki-latest-linktarget.sql.gz | ./build/graphzip parse-linktarget

# Translate the link table into dense ID pairs
gzip -dc data/enwiki-latest-pagelinks.sql.gz  | ./build/graphzip parse-pagelinks

# Cut the working subgraph, sort and deduplicate
./build/graphzip subset
./build/graphzip sort-edges

# Measure
./build/graphzip load
./build/graphzip bench
./build/graphzip verify
./build/graphzip compressed-bench
```

Wikipedia's IDs are sparse — page IDs reach roughly 130 million with about 5% in
use — so the first pass renumbers articles into a contiguous 0..n-1 space. That
matters twice over: array indexing replaces hashing, and the gaps between
adjacent targets stay small enough for gamma coding to pay off.

The dumps are MySQL `INSERT` statements, some lines several megabytes long. The
parser is a character-level scanner that splits tuples on commas except inside
quoted strings, honouring backslash escapes — titles contain commas,
apostrophes and backslashes, and a naive split silently misaligns every
subsequent field.

## Building

```bash
cmake -B build -G Ninja -DCMAKE_BUILD_TYPE=RelWithDebInfo
cmake --build build
./build/tests/tests
```

Requires a C++20 compiler. Catch2 is fetched at configure time.

## Notes

A 7-million-entry `std::unordered_map` was used initially to map Wikipedia page
IDs to dense IDs during the link translation pass. On a memory-constrained
machine this caused the pass to stall indefinitely — pinned CPU, no output —
as scattered heap nodes faulted on nearly every lookup. Replacing it with a flat
`std::vector` indexed directly by page ID, with a sentinel for unmapped entries,
made the working set contiguous and the pass completed at a steady rate. Same
asymptotics, entirely different behaviour under memory pressure.

## Next

- Batch the unary prefix scan in `read_gamma` using `countl_zero`
- Persist a `BitReader` across a traversal rather than constructing per call
- Interval encoding: runs of consecutive targets as start plus length
- Reference encoding: express a list as a delta against a similar earlier node,
  which is where the large gains in the literature come from
- Node reordering, which determines how well reference encoding can work

For comparison, the WebGraph framework (Boldi and Vigna) reports under 5 bits
per edge on graphs of this scale using the latter two techniques.