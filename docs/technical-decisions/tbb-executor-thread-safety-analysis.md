# Technical Decision Document: TBBExecutor Thread Safety Analysis

**Document ID**: TDD-2025-002
**Status**: Implemented
**Date**: 2025-09-09

## Executive Summary

Addressed potential thread safety concern in `TBBExecutor` where concurrent execution of `combinable::clear()` with `combinable::local()` could cause undefined behavior. Implemented const-correct stateless design eliminating the issue entirely.

## Problem Classification

**Issue**: Theoretical race condition between `reset_rngs()` and `parallel_evaluate()`
**Assessment**: Design improvement opportunity (not critical bug)
**Risk**: Very Low probability, Low-Medium impact

## Technical Foundation

Intel TBB documentation: *"Methods of class combinable are not thread-safe, except for local."* Concurrent access to `combinable::clear()` and `combinable::local()` constitutes undefined behavior.

## Decision

**Implemented**: Complete elimination of shared state
- Removed `combinable<std::mt19937> thread_rngs_` member
- Removed `reset_rngs()` method entirely
- Elimination of shared RNG state ensures thread safety by design

## Result

✅ Thread-safe by design
✅ Simplified API (no reset method needed)
✅ Const-correct implementation
✅ Eliminated theoretical race conditions
