# Introduction

NeoDB is a generic multi-disk SSD Cache for disaggregate storage systems.

- Cache Policy
  - It adopts a space utilization and read-write amplification optimized eviction algorithm instead of using traditional LRU or LFU algorithm.
  - The hotness-aware GC will help reserve hot items as much as possible.
- Performance
  - Close to 1X end-to-end write amplification (including the SSD internal GC)
- Space Utility
  - Close to 1X space amplification thus we are able to use up to 99% disk space.

# Features
- [x] Blocking API with concurrent access support.
- [x] File-based device manager
- [x] Multi device support
- [ ] Block layer device manager 
- [ ] Colored pointer for access frequency stats and GC
- [ ] Async API with io_uring
- [ ] Serve as a service