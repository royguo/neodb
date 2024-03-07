# Introduction

NeoDB is a generic multi-disk SSD Cache for disaggregate storage systems.

- Cache Policy
  - It adopts a space utilization and read-write amplification optimized eviction algorithm instead of using traditional LRU or LFU algorithm.
  - The hotness-aware GC will help reserve hot items as much as possible.
- Performance
  - Close to 1X end-to-end write amplification (including the SSD internal GC)
- Space Utility
  - Close to 1X space amplification thus we are able to use up to 99% disk space.

# Build

# Design
- Async API with coroutine support.
  - Thus we can use minimum CPU resource to leverage the maximum disk bandwidth.
- Single stream append-only data model.
  - This design helps us reduce end-to-end write amplification and get better performance.
- Colored pointer for access frequency.
  - We reserve more than 10 bits for each index pointer, so we can have better understanding of the access frequency.

# Use Cases