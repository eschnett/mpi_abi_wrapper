# `dev/handle-map-bench/`

Would a sorted array with binary or interpolation search beat a hash table for the
implementation → ABI predefined-handle map? That map is on the hot path — every
out-handle and every user-op trampoline — and the datatype class is the big one.

```sh
cc -std=c11 -O2 -o bench bench.c && ./bench
```

Two key distributions, because they behave nothing alike. **mpich** is the 77 real
predefined `MPI_Datatype` values from MPICH's header, `0x0c000000`..`0x8c000004`: a
kind field in the high bits and dense low bits, so severely non-uniform. **ompi** is
Open MPI's addresses of consecutive static objects, modelled as a base plus a fixed
stride, so nearly uniform. Two access patterns: `hot` reuses one key (an
`MPI_Send` loop with one datatype), `sweep` round-robins over all of them.

## Results (macOS arm64, clang -O2, ns per lookup)

| | mpich hot | mpich sweep | ompi hot | ompi sweep |
|---|---|---|---|---|
| perfect hash | **1.104** | **1.093** | **1.103** | **1.085** |
| open-addressing hash | 1.094 | 1.355 | 1.099 | 1.532 |
| sorted + binary search | 3.879 | 3.724 | 3.893 | 3.717 |
| sorted + interpolation search | **88.067** | 82.523 | 1.367 | 1.633 |

## Conclusions

**No — sorted arrays are slower.** Binary search costs 3.4× a hash: seven dependent
comparisons, each an unpredictable branch, and the branch predictor cannot help
because the path depends on the key.

**Interpolation search is a trap on the distribution that actually occurs.** 88 ns,
eighty times slower, on the real MPICH key set. O(log log n) assumes uniformly
distributed keys; MPICH's are one value at `0x0c000000`, a dense cluster around
`0x4c00xxxx`, and one at `0x8c000004`. Interpolation guesses badly, degenerates
toward a linear scan, and pays a floating-point divide at every step. It looks fine
on Open MPI's uniform addresses (1.37) and is still no better than a hash there.

**A perfect hash wins, and it is what the design should use.** Because the whole key
set is known at initialization, a multiplier can simply be searched for until no two
keys collide — found immediately for both distributions here. The lookup is then one
multiply, one shift, one load, one compare, with **no probe loop**: that is why it
holds 1.09 ns even on the sweep where open addressing degrades to 1.36–1.53 ns. The
probe loop is a loop-carried unpredictable branch; removing it removes the only part
that was data-dependent.

At ~1.09 ns (about 4 cycles) this is one L1 load plus a compare. Going faster would
mean exploiting per-implementation structure — MPICH's low bits as a direct index —
at the cost of generality.

## Note on the fallback

Perfect-hash construction must be bounded and must fail *loudly at initialization*
rather than degrading to probing at run time, or the branch it exists to remove comes
back. Widen the table and retry; if that still fails, refuse in
`mpiwrapper_get_vtable` with a diagnostic. With 104 keys in 1024 slots a random
multiplier succeeds with high probability, so this is contingency handling.
