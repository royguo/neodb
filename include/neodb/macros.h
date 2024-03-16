#pragma once

// Minimal IO size
#define IO_PAGE_SIZE 4096

// Best IO size for the target device
#define OPTIMIZED_READ_IO_SIZE (256UL << 10)

#define IO_FLUSH_SIZE (512UL << 10)

#define MAX_KEY_SIZE (1024)

#define MAX_VALUE_SIZE (32UL << 20)