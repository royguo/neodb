# Introduction

NeoDB is a generic multi-disk SSD Cache for disaggregate storage systems.

- Cache Policy
  - It adopts a space utilization and read-write amplification optimized eviction algorithm instead of using traditional LRU or LFU algorithm.
  - The hotness-aware colored pointer will help reserve hot items as much as possible.
- Performance
  - Close to 1X end-to-end write amplification (Inlcuding the SSD internal GC)
  - Close to 2X end-to-end read amplification.
- Space Utility
  - Close to 1X space amplification thus we are able to use up to 99% disk space.


# Build

# Contribution

# Use Cases
