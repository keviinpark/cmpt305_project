# Tests

This folder contains basic tests for the CPU pipeline simulator.

## Suggested test types
- Build test: project compiles with `make`.
- Smoke test: simulator runs with a small trace file.
- Behavior test: expected cycle counts or retired instruction counts for fixed inputs.

## Example manual run
From `src/`:

```bash
make
./cpu_simulator traces/sample.trace 0 100 5
```
