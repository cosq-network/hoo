# Hooc Standard Library Design

Last Updated: 2026-05-22

This document defines the planned and active standard-library surface under the `hoo` namespace, aligned with the current HVM core profile.

## 1. Design Principles

1. Keep HVM core ISA minimal
2. Implement rich behavior in runtime/library modules
3. Keep APIs straightforward and testable
4. Prefer stable module boundaries over opcode proliferation

## 2. Namespace and Core Types

Primary namespace: `hoo`

Current core runtime-backed types:

- `hoo.String`
- `hoo.Array`

Language-native arrays (`T[]`) remain first-class and type-safe.

## 3. Module Status

## 3.1 `hoo.io`

Status: partially implemented

Available baseline operations:

- `hoo.print(...)`
- `hoo.println(...)`
- `hoo.readline()`
- `hoo.readchar()`

Planned expansions:

- file handling
- directory/path utilities
- richer console/stream helpers

## 3.2 `hoo.math`

Status: partial/incremental

Focus:

- numeric utility functions
- power/roots/trigonometry
- deterministic and seeded random utilities

## 3.3 `hoo.net`

Status: partial/incremental

Focus:

- URL helpers
- HTTP client baseline
- optional socket abstractions as runtime maturity grows

## 3.4 `hoo.time`

Status: planned

Focus:

- duration and instant abstractions
- local date/time structures
- formatting/parsing and timezone support

## 4. Collections Policy

Generic collections (`List/Map/Set` class families) are not a short-term priority.

Preferred current approaches:

- `hoo.Array` for flexible dynamic storage
- native `T[]` for homogeneous typed arrays
- map syntax/type support from grammar where needed

## 5. HVM Alignment

Standard library growth should rely on:

- runtime calls and module APIs
- FFI bridge (`CALLHOST`, `CALLNATIVE`, `LOADLIB`, `GETSYM`)

and should **not** force core ISA growth unless grammar-level semantics require it.

## 6. Incremental Delivery Plan

1. Harden `hoo.io` practical file/path APIs
2. Expand `hoo.math` numerics and random features
3. Stabilize `hoo.net` client-centric APIs
4. Add `hoo.time` foundation types

Each step should ship with:

- examples
- unit/integration tests
- behavior notes for JIT/AOT module workflows

## 7. Compatibility Notes

- Keep public module APIs backward-conscious
- If a feature needs VM capabilities beyond core, stage it through optional HVM extension profiles first
