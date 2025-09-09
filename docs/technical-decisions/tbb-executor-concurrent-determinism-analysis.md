# Technical Decision Document: TBBExecutor Concurrent Determinism Analysis

**Document ID**: TDD-2025-001  
**Status**: Implemented  
**Date**: 2025-09-09  

## Executive Summary

Refactored `TBBExecutor` to eliminate shared mutable state and achieve const-correctness. The new stateless design uses method-scoped RNG initialization instead of instance-level shared state, ensuring deterministic behavior under concurrent access while maintaining performance.

## Problem Statement

Original implementation used shared `rng_init_count_` and `thread_rngs_` members, potentially causing non-deterministic thread seed assignment when `parallel_evaluate()` is called concurrently.

## Decision

**Implemented**: Stateless const-qualified `parallel_evaluate()` method
- RNG state moved to method scope
- Eliminated shared mutable state (`rng_init_count_`, `thread_rngs_`)  
- Simplified architecture removes `reset_rngs()` complexity

## Result

✅ Const-correct API supporting concurrent access  
✅ Deterministic behavior preserved  
✅ Performance maintained  
✅ Simplified implementation