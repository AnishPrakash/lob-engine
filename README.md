# LOB Engine - Ultra-Low-Latency Limit Order Book

[![CI](https://github.com/AnishPrakash/lob-engine/actions/workflows/ci.yml/badge.svg)](https://github.com/AnishPrakash/lob-engine/actions/workflows/ci.yml)
[![License](https://img.shields.io/badge/license-MIT-blue)](LICENSE)
[![asciicast](https://asciinema.org/a/zHn3p2k8SuuHqsXw.svg)](https://asciinema.org/a/zHn3p2k8SuuHqsXw)

## Performance

| Metric | Value |
|---|---|
| Throughput | 27M orders/sec |
| P50 Latency | 42 ns |
| P99 Latency | 250 ns |
| Cache-Miss Rate | < 0.5% |

## Architecture

* Zero-copy ITCH 5.0 binary parser
* 15M-slot monolithic memory pool (zero hot-path heap alloc)
* Intrusive linked lists with 32-bit indices
* Bitset price level tracker with `__builtin_ctzll`
* Self-match prevention
* FIX 4.2 execution report output
* ncurses terminal visualizer

## Quick Start

```bash
cmake -B build -DCMAKE_BUILD_TYPE=Release -DCMAKE_TOOLCHAIN_FILE=$VCPKG_ROOT/scripts/buildsystems/vcpkg.cmake
cmake --build build -j$(nproc)

./build/lob_engine data/sample.itch
