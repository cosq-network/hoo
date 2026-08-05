# Hoo Archive (`.ha`) Format Specification

The `.ha` (Hoo Archive) format is the standard distributable artifact for multi-file Hoo programs. It packages compiled `.ho` module payloads, dependency metadata, and symbol lookup tables into a single executable and highly-compressible container.

## 1. Container Structure
The `.ha` file is a ZIP-compatible archive, containing intermediate module files (`.ho`) and metadata (`archive.json`). 

Instead of compressing individual files (like standard ZIP or Java `.jar` files do), Hoo Archives store internal payloads **uncompressed** and apply **Zstandard (Zstd)** compression to the **entire archive stream**. 

### Internal Layout
```text
META-INF/hoo/archive.json
modules/main.ho
modules/pkg/math_utils.ho
modules/pkg/internal/parser.ho
```
- **`META-INF/hoo/archive.json`**: The central manifest containing dependency graphs, module metadata, and a strict symbol lookup table.
- **`modules/*.ho`**: Low-level, physical-silicon-ready 64-bit RISC module payloads.

## 2. Comparison: `.ha` (Hoo Archive) vs `.jar` (Java Archive)

While `.ha` serves a similar purpose to Java's `.jar` format (providing a single distributable artifact), their internal architectures optimize for entirely different execution philosophies.

| Feature | Hoo Archive (`.ha`) | Java Archive (`.jar`) |
| :--- | :--- | :--- |
| **Container Structure** | ZIP-compatible format. | ZIP-compatible format. |
| **Compression** | Stores internal files *uncompressed*, but compresses the **entire archive stream** using **Zstandard** (balances compression ratio and fast JIT decompression). | Uses standard ZIP (Deflate) compression on **individual files** within the archive. |
| **Internal Payloads** | Contains compiled **`.ho` files**. These are low-level, physical-silicon-ready 64-bit RISC module payloads. | Contains compiled **`.class` files**. These are high-level, semantic bytecode instructions. |
| **Manifest / Metadata** | Uses **`META-INF/hoo/archive.json`** (minified JSON) storing dependency graphs, entry points, and strict symbol lookup tables for fast loading. | Uses **`META-INF/MANIFEST.MF`** (custom text-based key-value format) specifying the Main-Class, classpath, and package metadata. |
| **Execution** | Loaded into the **HVMJIT** (an LLVM ORC-based dynamic binary translator) which topologically orders modules and rejects circular dependencies during resolution. | Loaded into the **JVM** which incrementally loads and verifies `.class` files on-demand during runtime. |
| **Resolution** | Files are compiled sequentially, packaged into `.ha`, and executed as a monolithic payload. | Highly dynamic classpaths allow multiple overlapping `.jar` files to resolve dependencies at runtime. |

### Which is better?

#### 1. Compression: `.ha` is Superior
The **`.ha`** format achieves significantly better compression ratios and faster decompression speeds:
* **Algorithm (Zstandard vs. Deflate):** `.ha` uses **Zstandard**, which heavily outperforms the legacy **Deflate** algorithm used by `.jar` files in both compression density and decompression speed.
* **Solid vs. Per-File Compression:** A `.jar` file compresses each `.class` file individually. A `.ha` file stores `.ho` payloads uncompressed internally, and then compresses the **entire archive stream** at once (similar to a `.tar.gz`). This allows the Zstandard algorithm to find repeating patterns and redundant code *across* different files, resulting in significantly smaller overall file sizes.

#### 2. Loading for Execution: A Trade-off
Which format is "better" for loading depends entirely on the environment's execution model:

**`.ha` (Ahead-of-Time / JIT Optimized)**
* **How it loads:** The `HooArchiveLoader` reads the single `archive.json` manifest, instantly knows the dependency graph of all modules and their symbols, and loads everything into memory in the exact topological order required by the JIT.
* **Pros:** Extremely fast and predictable execution once loaded. The loader doesn't have to scan the archive directory structure or pause execution later to find missing symbols.
* **Cons:** Slower initial startup time for massive applications, because the entire archive must be decompressed and linked into the LLVM JIT before the `main` function starts.

**`.jar` (Lazy / On-Demand Optimized)**
* **How it loads:** The JVM reads the `MANIFEST.MF` to find the entry point. As the program executes, if it hits a class it hasn't seen before, it halts, searches the `.jar` for that specific `.class` file, decompresses it, verifies it, and loads it.
* **Pros:** Blazing fast initial startup time for massive applications (e.g., enterprise Spring Boot servers), because it only loads the exact fraction of code needed to start.
* **Cons:** Causes "JIT warmup" latency. The application will experience micro-stutters during execution as it pauses to search the archive and lazily decompress new files.

### Summary
* **For Compression:** `.ha` wins decisively thanks to Zstandard whole-archive compression.
* **For Loading:** `.ha` is better for systems-level performance (zero runtime stuttering, fully resolved memory), while `.jar` is better for instantly booting up massive applications through lazy loading.

### Dependency ordering contract

The archive manifest records module dependencies by name. During loading, the
HVM bundle resolves each module's own dependency edges with a DFS traversal,
emits dependencies before dependents, and rejects a real cycle such as
`A -> B -> C -> A`. A missing optional dependency is ignored for ordering;
required-module availability is validated by the loader.
