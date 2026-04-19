# Hooc Development Roadmap

This document outlines the planned development trajectory for the Hooc programming language and compiler. It provides a timeline of features, priorities, and milestones for future development.

**Last Updated:** April 19, 2026

## Vision and Goals

### Long-term Vision

Hooc aims to be a modern, productive programming language that combines:
- **Safety**: Strong static typing with nullable types and memory safety
- **Performance**: Native code generation via LLVM
- **Expressiveness**: Clean, intuitive design with classes
- **Simplicity**: Clean, intuitive design without unnecessary complexity
- **Interoperability**: Easy FFI with C/C++ and other languages

### Core Principles

1. **Developer Productivity**: Features that reduce boilerplate and improve clarity
2. **Type Safety**: Catch errors at compile time
3. **Zero-Cost Abstractions**: High-level features with minimal runtime overhead
4. **Gradual Adoption**: Can start simple and use advanced features as needed
5. **Excellent Tooling**: First-class IDE support, debugger, and package manager

## Current Status (Phase 8)

- ✅ Core language features (Lexer/Parser/AST)
- ✅ Object-oriented programming (Parsing and AST support)
- ✅ Simplified type system (Generics removed, `std::any` fallback)
- ✅ Module-level variables and constants (`var`, `const`)
- ✅ Dynamic Global Initialization (`__hoo_init` system)
- ✅ Runtime library (ARC, Strings, Arrays, IO)
- ✅ Modernized CLI (HooCLI with C++17, AOT reserved flags)
- ✅ Stable JIT Engine (Shared ThreadSafeContext)
- ✅ Module System Design ([docs/module-system-design.md](module-system-design.md))
- ✅ Compound assignment operators (`+=`, `-=`, `*=`, `/=`, `%=`, `<<=`, `>>=`)
- ✅ Increment/decrement operators (`++`, `--`)
- ✅ Multiline string support (`"""..."""`)
- ✅ Function modifiers (`public`, `private`, `async`) for member functions
- 🟡 Standard library implementation in progress (Basic IO and Strings complete)
- 🟡 Module resolution implementation in progress (Built-in `hoo` resolution complete)

## Development Phases

### Phase 8: Standard Library and AOT Infrastructure (Q2 2026)

**Priority:** High
**Duration:** 2-3 months
**Focus:** Essential standard library modules and binary output preparation.

#### Deliverables

**Collections Module** (`hoo.collections`)
- [ ] `List` - Dynamic array/list using built-in array types
- [ ] `Map` - Hash map with string keys
- [ ] `Set` - Hash set for unique values
- [ ] `Queue` - FIFO queue
- [ ] `Stack` - LIFO stack
- [ ] Iterator protocol
- [ ] Range types
- [ ] Collection utilities (sort, filter, map, reduce)

**IO Module** (`hoo.io`)
- [x] `hoo.print()` - Print without newline (implemented)
- [x] `hoo.println()` - Print with newline (implemented)
- [x] `hoo.readline()` - Read line from stdin (implemented)
- [x] `hoo.readchar()` - Read single character (implemented)
- [x] CLI components (`HooCLI`, `IOProvider`, `DefaultIOProvider`)
- [ ] AOT Bytecode Generation - Compile `.hoo` source to `.ho` bytecode files (CLI reserved)
- [ ] Native AOT Execution - Direct execution of `.ho` bytecode files (CLI reserved)
- [x] Module System Design - [docs/module-system-design.md](module-system-design.md)
- [ ] `File` - File reading/writing
- [ ] `Directory` - Directory operations
- [ ] `Console` - Standard input/output
- [ ] `Stream` - Stream-based I/O
- [ ] Path utilities
- [ ] File system operations

**Module System Implementation**
- [x] Rebranded built-in namespace from `std` to `hoo`
- [x] Basic selective import parsing (`from ... import ...`)
- [ ] Filesystem-based `ModuleResolver` (mapping logical paths to files)
- [ ] Cross-module symbol mangling (`_H_<module>_<symbol>`)
- [ ] Recursive dependency discovery and compilation
- [ ] Package manifest (`hooc.json`) support

#### Success Criteria
- All standard library modules tested
- Documentation for each module
- Performance benchmarks established
- Examples for common use cases

---

### Phase 9: Advanced OOP and Compiler Completion (Q3 2026)

**Priority:** High
**Duration:** 1-2 months
**Focus:** Full implementation of existing grammar features and backend optimizations.

#### Deliverables

**Object-Oriented Features** (Grammar supported)
- [ ] Class Inheritance (`extends`) implementation in codegen
- [ ] Virtual method tables (VTable) for inheritance
- [ ] Class Modifiers (Singleton, Actor, Immutable) logic

**Advanced Language Support**
- [ ] Type-safe Dynamic Arrays (moving beyond `std::any` for primitives)
- [ ] String interpolation syntax (`"Hello ${name}"` with runtime substitution)
- [ ] Complex function expressions (function pointers)
- [ ] Full AOT Linking and Bytecode emission

**Optimizations**
- [ ] LLVM Optimization pass integration
- [ ] Dead code elimination
- [ ] Inlining support

---

### Phase 10: Error Handling and Future Extensions (Q4 2026+)

**Priority:** Medium
**Duration:** Ongoing
**Focus:** New language syntax and robust error management.

#### Planned Deliverables

**Exception System** (Grammar Update Required)
- [ ] `try-catch` syntax
- [ ] Exception types and hierarchy
- [ ] Stack unwinding
- [ ] Custom exception classes

**Result Types** (Grammar Update Required)
- [ ] `match` syntax for Result patterns
- [ ] Error propagation operators (`?`)

**Long-term Features (2028+)**
- [ ] Dependent types (exploration)
- [ ] Effect systems
- [ ] Linear types for resource management
- [ ] Compile-time metaprogramming
- [ ] Macro system

### Ecosystem Development
- Rich standard library expansion
- Web framework
- Game development framework
- Data science libraries
- Machine learning integration

### Research and Innovation
- Novel type system features
- Advanced optimization techniques
- Integration with emerging technologies
- Community-driven feature development

## Community and Governance

### Open Source Development
- Public repository with open issues
- Contributor guidelines
- Code review process
- Regular release cycle
- Semantic versioning

### Community Engagement
- Official forum/Discord
- Regular blog posts
- Conference presentations
- Tutorial videos
- Example projects

### Documentation
- Language reference manual
- Standard library documentation
- Tutorial series (beginner to advanced)
- Design rationale documents
- Migration guides

## Release Schedule

### Version 0.x (Current)
- Experimental releases
- Breaking changes allowed
- Focus on core features
- Community feedback

### Version 1.0 (Target: Late 2027)
- Stable API
- Complete standard library
- Production-ready tooling
- Comprehensive documentation
- Backward compatibility guarantees

### Post-1.0
- Semantic versioning
- Long-term support (LTS) releases
- Regular feature releases
- Security updates

## Metrics and Success Criteria

### Adoption Metrics
- GitHub stars and forks
- Package downloads
- Active contributors
- Production use cases
- Community size

### Quality Metrics
- Test coverage (>90%)
- Performance benchmarks
- Bug report resolution time
- Documentation completeness
- User satisfaction surveys

### Technical Metrics
- Compilation speed
- Runtime performance
- Memory efficiency
- Binary size
- Startup time

## Risk Mitigation

### Technical Risks
- **LLVM dependency**: Stay current with LLVM releases
- **Breaking changes**: Maintain compatibility layers
- **Performance**: Regular benchmarking and profiling
- **Complexity**: Phased rollout of advanced features

### Community Risks
- **Adoption**: Focus on developer experience and tooling
- **Contributions**: Clear contribution guidelines
- **Burnout**: Sustainable development pace
- **Competition**: Differentiate with unique features

## How to Contribute

Interested in contributing to Hooc development? Here's how:

1. **Review the roadmap** - Pick a feature that interests you
2. **Check implementation status** - See [implementation-status.md](implementation-status.md)
3. **Discuss your approach** - Open an issue to discuss design
4. **Follow development guide** - See [CLAUDE.md](../CLAUDE.md)
5. **Submit pull request** - Include tests and documentation

### Priority Areas for Contributors
- Standard library modules (Phase 8)
- Error handling implementation (Phase 9)
- Documentation and examples
- Bug fixes and optimizations

## Conclusion

This roadmap represents an ambitious but achievable vision for Hooc. The phased approach allows for incremental progress while maintaining stability. Community feedback will shape priorities and timelines.

**The journey to a modern, powerful, and productive programming language continues.**

---

**Questions or suggestions?** Open an issue or join the discussion!

## See Also
- [Implementation Status](implementation-status.md) - Current status
- [Features Guide](features.md) - Feature documentation
- [Grammar Specification](grammar.md) - Language grammar
- [CLAUDE.md](../CLAUDE.md) - Developer guide
