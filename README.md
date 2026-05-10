# TickEngine

A C++ strategy engine for live trading Fubon Python API.
Two-process design: Python adapter handles the broker SDK, C++ engine runs the
strategy core. They communicate via ZMQ + JSON.

## Why this project

I want to run strategies I wrote during my quant research intern period on a
C++ engine I built myself. The goal: let real strategy requirements drive the
engine design, instead of designing in a vacuum.

## Architecture

See [DESIGN.md](DESIGN.md) for the full design.

Short version: Python owns the broker connection. C++ owns the strategy, risk,
and position tracking. They talk through ZMQ with JSON messages.

## Build

```bash
make
```

Requires Linux. CMake-based — see `CMakeLists.txt` for details.
