# nme_bench_jobs

Google Benchmark harness for the `nme::JobSystem` work-stealing scheduler. One
`JobSystem` and timer are started once in `main` and shared by every case, so
pool start-up never pollutes a measurement.

Build via `nme_add_benchmark(jobs SOURCES job_bench.cpp)` and run in a Release
config (`RelWithDebInfo` is fine). Numbers are only meaningful under optimization.

## Reading the columns

- **Time**: wall clock per iteration. For Throughput/Grain one iteration is one
  dispatch-and-wait; for FrameSim it is one hand-timed frame (`UseManualTime`).
- **CPU**: total process CPU across all workers (`MeasureProcessCPUTime`), not
  just main. `CPU / Time` ≈ cores kept busy.
- **items_per_second**: jobs/s (divides by wall time, `UseRealTime`).
- Aggregates come from `--benchmark_repetitions`. Judge spikes by **mean vs
  median** and absolute **stddev**, never `cv%` alone when the mean is moving.

## Modes

Each mode is selected with `--benchmark_filter`. The standard invocation adds 15
repetitions and prints aggregates only:

### Throughput: granularity sweep

```
nme_bench_jobs --benchmark_filter=Throughput \
               --benchmark_repetitions=15 --benchmark_report_aggregates_only=true
```

Sweeps `{jobs, work}` from many-tiny to few-fat. The `work=0` row is a pure
dispatch-throughput meter; the fat rows are work-bound. Dispatch uses
`parallelFor` (recursive split), swap the call to `runN` in `BM_JobThroughput`
to A/B against flat chunking.

### Grain: dispatch scaling, recursive vs flat

```
nme_bench_jobs --benchmark_filter=Grain \
               --benchmark_repetitions=15 --benchmark_report_aggregates_only=true
```

Fixed workload, varying grain (items per chunk/leaf), one row per grain for both
`BM_GrainRecursive` (`parallelFor`) and `BM_GrainFlat` (`runN`). Pair rows by
grain and chart median/cv vs grain: recursive holds flat where the flat chunker
climbs from building all chunks on the submitter. Fixed count/work are constants
at the top of the block; push grain finer (or count higher) to sharpen the split.

### FrameSim: dependency-phase frame simulation

```
nme_bench_jobs --benchmark_filter=FrameSim \
               --benchmark_repetitions=15 --benchmark_report_aggregates_only=true
```

Four sequential phases (anim → physics → culling → render) plus one fat
non-splittable straggler, scaled by `scale`. One iteration = one frame.
Extra counters: **worst_ms** (real worst single frame in the run) and
**worst/mean** (tail spread; 1.0 = flat). `worst_ms` is an extremum, so trust the
`worst/mean` *median* over any single value. Note that four barriers bound
occupancy structurally, each phase drains the pool before the next starts.

## Correctness

```
nme_bench_jobs --verify
```

Not a benchmark: asserts every job runs exactly once and each slot is written by
its own job, then exits. Belongs in the test target long-term; kept here so the
file is a drop-in.

## Notes

- Sub-millisecond frames (e.g. FrameSim scale 1) sit near the measurement floor;
  fixed OS/scheduler jitter dominates there and inflates `cv`/`worst/mean`. Judge
  the scheduler on the larger cases.
- Profile and benchmark in separate runs, Tracy zone overhead perturbs the
  fine-grained rows most.
- Snapshot a baseline with `--benchmark_out=baseline.json --benchmark_out_format=json`
  and diff later runs with Google Benchmark's `tools/compare.py`.