# Introduction

NeoDB is a generic multi-disk SSD Cache for disaggregate storage systems.

- Cache Policy
  - It adopts a space utilization and read-write amplification optimized eviction algorithm instead of using traditional LRU or LFU algorithm.
  - The hotness-aware GC will help reserve hot items as much as possible.
- Performance
  - Close to 1X end-to-end write amplification (including the SSD internal GC)
- Space Utility
  - Close to 1X space amplification thus we are able to use up to 99% disk space.
- NeoFS Core
  - NeoDB is the low-level storage engine for NeoFS (not open-source yet)


![NeoDB and NeoFS](https://github.com/royguo/neodb/blob/main/docs/neodb.png)


# Features
- [x] Blocking API with concurrent access support.
- [x] File-based device manager
- [x] Multi device support
- [ ] Block layer device manager 
- [ ] Colored pointer for access frequency stats and GC
- [ ] Async API with io_uring
- [ ] Serve as a service


# Example

```
    std::unique_ptr<NeoDB> db_;

    // First device
    StoreOptions store_options1;
    store_options1.name_ = "store1";
    store_options1.device_path_ =
        FileUtils::GenerateRandomFile(device_prefix_, FLAGS_device_capacity_gb << 30);
    store_options1.device_zone_capacity_ = FLAGS_zone_capacity_mb << 20;
    store_options1.device_capacity_ = FLAGS_device_capacity_gb << 30;

    // Second device
    StoreOptions store_options2;
    store_options2.name_ = "store2";
    store_options2.device_path_ =
        FileUtils::GenerateRandomFile(device_prefix_, FLAGS_device_capacity_gb << 30);
    store_options2.device_zone_capacity_ = FLAGS_zone_capacity_mb << 20;
    store_options2.device_capacity_ = FLAGS_device_capacity_gb << 30;

    // Init database with two devices.
    DBOptions db_options_;
    db_options_.store_options_list_.emplace_back(std::move(store_options1));
    db_options_.store_options_list_.emplace_back(std::move(store_options2));
    db_ = std::make_unique<NeoDB>(db_options_);


    // Usage
    std::string key = StringUtils::GenerateRandomString(10);
    std::string value = StringUtils::GenerateRandomString(1000);
    auto s = db_->Put(key, value);
```


Check `include/neodb/neodb.h` for the full API list.
