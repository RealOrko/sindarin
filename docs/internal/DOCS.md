# Documentation Consolidation Plan

This document analyzes the current state of documentation in the Sindarin project and provides a specification for consolidating it into a well-organized structure.

---

## Current State Analysis

### Documentation Inventory

| File | Location | Type | Status |
|------|----------|------|--------|
| README.md | root | User guide | **Keep** - Needs trimming |
| CLAUDE.md | root | AI instructions | **Keep** - Update references |
| ARRAY.md | root | Implementation plan | **Consolidate** - Mostly obsolete |
| IO.md | root | Design spec | **Consolidate** - Merge with docs/FILE_IO.md |
| MEMORY.md | root | Specification | **Move** - Primary spec |
| TIME.md | root | Specification | **Move** - Implemented feature |
| DATE.md | root | Draft specification | **Move** - Preserve as draft |
| INTEROP.md | root | Draft specification | **Move** - Preserve as draft |
| OPTIMIZATIONS.md | root | Performance guide | **Move** - Internal docs |
| EXPLORE.md | root | Test coverage | **Move** - Internal docs |
| ISSUES.md | root | Bug tracking | **Move** - Internal docs |
| REFACTOR.md | root | Refactoring plan | **Move** - Internal docs |
| RUNTIME.md | root | Implementation notes | **Move** - Internal docs |
| BENCHMARKS.md | root | Results | **Move** - Keep with benchmarks |
| docs/ARRAYS.md | docs | User spec | **Update** - Primary spec |
| docs/FILE_IO.md | docs | User spec | **Update** - Primary spec |
| docs/MEMORY.md | docs | User spec | **Superseded** - Use root MEMORY.md |
| docs/type_checker_error_audit.md | docs | Audit | **Move** - Internal docs |
| docs/optimization_report.md | docs | Report | **Move** - Internal docs |
| docs/string_interpolation_analysis.md | docs | Analysis | **Move** - Internal docs |
| docs/format_specifiers.md | docs | Proposal | **Review** - May not be implemented |
| benchmark/SPEC.md | benchmark | Spec | **Keep** - Already in right place |

---

## Document Categories

### Category 1: Language Specification Documents

These form the **official specification** of the Sindarin language and should be the single source of truth.

| Feature | Primary Source | Secondary/Obsolete | Action |
|---------|---------------|-------------------|--------|
| **Arrays** | docs/ARRAYS.md | ARRAY.md (root) | Verify docs/ARRAYS.md is complete; archive ARRAY.md implementation notes |
| **File I/O** | docs/FILE_IO.md | IO.md (root) | Verify docs/FILE_IO.md matches implementation; preserve IO.md design decisions |
| **Memory** | MEMORY.md (root) | docs/MEMORY.md | Use root version (more comprehensive); update or remove docs version |
| **Time** | TIME.md | - | Move to docs/, verify implementation |
| **Strings** | (README.md section) | - | Extract to standalone docs/STRINGS.md |
| **Types** | (README.md section) | - | Extract to standalone docs/TYPES.md |

### Category 2: Draft Specifications (Future Features)

These describe features **not yet implemented** but should be preserved for future development.

| Document | Content | Action |
|----------|---------|--------|
| DATE.md | Date type specification | Move to docs/drafts/DATE.md |
| INTEROP.md | C interoperability | Move to docs/drafts/INTEROP.md |
| docs/format_specifiers.md | Format specifier syntax | Move to docs/drafts/ if not implemented |

### Category 3: Internal Development Documents

These are valuable for developers but should not be in the root or main docs.

| Document | Content | Action |
|----------|---------|--------|
| OPTIMIZATIONS.md | Performance optimization guide | Move to docs/internal/OPTIMIZATIONS.md |
| REFACTOR.md | Codebase restructuring proposal | Move to docs/internal/REFACTOR.md |
| RUNTIME.md | Runtime function inlining | Move to docs/internal/RUNTIME.md |
| EXPLORE.md | Test coverage analysis | Move to docs/internal/TESTING.md |
| ISSUES.md | Known bugs | Move to docs/internal/ISSUES.md |
| docs/type_checker_error_audit.md | Error message audit | Move to docs/internal/ |
| docs/optimization_report.md | Optimization results | Move to docs/internal/ |
| docs/string_interpolation_analysis.md | Technical analysis | Move to docs/internal/ |

### Category 4: Project-Level Documents

These remain in the root directory.

| Document | Purpose | Action |
|----------|---------|--------|
| README.md | Project overview, quick start | Trim to essentials, link to docs/ |
| CLAUDE.md | AI assistant instructions | Update to reference new docs/ structure |
| BENCHMARKS.md | Performance results | Move to benchmark/BENCHMARKS.md |

---

## Proposed Directory Structure

```
./
├── README.md                    # Concise project overview
├── CLAUDE.md                    # AI instructions
│
├── docs/
│   ├── README.md               # Documentation index
│   │
│   ├── language/               # Language specification
│   │   ├── OVERVIEW.md         # Language overview and philosophy
│   │   ├── TYPES.md            # Data types reference
│   │   ├── ARRAYS.md           # Array operations (from current docs/)
│   │   ├── STRINGS.md          # String operations (extract from README)
│   │   ├── FILE_IO.md          # File I/O (from current docs/)
│   │   ├── MEMORY.md           # Memory management (from root)
│   │   └── TIME.md             # Time operations (from root)
│   │
│   ├── drafts/                 # Unimplemented feature specs
│   │   ├── DATE.md             # Date type (NOT IMPLEMENTED)
│   │   ├── INTEROP.md          # C interop (NOT IMPLEMENTED)
│   │   └── FORMAT_SPECIFIERS.md # String format specifiers
│   │
│   └── internal/               # Developer documentation
│       ├── ARCHITECTURE.md     # Compiler architecture
│       ├── OPTIMIZATIONS.md    # Performance guide
│       ├── REFACTOR.md         # Restructuring plans
│       ├── TESTING.md          # Test coverage
│       ├── ISSUES.md           # Known issues
│       └── AUDITS.md           # Type checker, code gen audits
│
└── benchmark/
    ├── SPEC.md                 # Benchmark specification
    └── RESULTS.md              # Performance results
```

---

## Validation Checklist

Before consolidation, each specification document must be verified against the implementation:

### Arrays (docs/ARRAYS.md)

- [ ] Verify all methods listed are implemented: `push`, `pop`, `insert`, `remove`, `reverse`, `clone`, `concat`, `indexOf`, `contains`, `join`, `clear`, `length`
- [ ] Verify slicing syntax works: `arr[1..4]`, `arr[..3]`, `arr[2..]`, `arr[..]`, step slicing
- [ ] Verify negative indexing works
- [ ] Verify range literals work: `1..6`
- [ ] Verify spread operator works: `{...arr, 4, 5}`
- [ ] Verify for-each iteration works
- [ ] Verify array equality works: `arr1 == arr2`
- [ ] Note: Arrays of lambdas require parentheses: `(fn(): int)[]`

### File I/O (docs/FILE_IO.md)

- [ ] Verify TextFile static methods: `open`, `exists`, `readAll`, `writeAll`, `copy`, `move`, `delete`
- [ ] Verify TextFile instance methods: `readLine`, `readAll`, `readLines`, `readWord`, `readChar`, `writeLine`, `isEof`, `hasChars`, `close`
- [ ] Verify BinaryFile static methods: `open`, `exists`, `readAll`, `writeAll`, `copy`, `move`, `delete`
- [ ] Verify BinaryFile instance methods: `readByte`, `readBytes`, `readAll`, `writeByte`, `writeBytes`, `isEof`, `hasBytes`, `close`
- [ ] Verify byte array methods: `toString`, `toHex`, `toBase64`, `toBytes` (on strings)
- [ ] Verify Bytes static methods: `fromHex`, `fromBase64`

### Memory (MEMORY.md)

- [ ] Verify `shared` function modifier works
- [ ] Verify `private` function modifier works (only primitives can return)
- [ ] Verify `shared` block modifier works
- [ ] Verify `private` block modifier works
- [ ] Verify `shared for`, `shared while`, `shared for-each` work
- [ ] Verify `as val` copy semantics work for arrays
- [ ] Verify `as ref` heap allocation works for primitives
- [ ] Verify `.clone()` creates independent copy

### Time (TIME.md)

- [ ] Verify Time.now() works
- [ ] Verify Time.utc() works
- [ ] Verify Time.fromMillis() works
- [ ] Verify Time.fromSeconds() works
- [ ] Verify Time.sleep() works
- [ ] Verify instance getters: `millis`, `seconds`, `year`, `month`, `day`, `hour`, `minute`, `second`, `weekday`
- [ ] Verify formatting: `format`, `toIso`, `toDate`, `toTime`
- [ ] Verify arithmetic: `add`, `addSeconds`, `addMinutes`, `addHours`, `addDays`, `diff`
- [ ] Verify comparison: `isBefore`, `isAfter`, `equals`

### Strings (extract from README.md)

- [ ] Document all string methods: `toUpper`, `toLower`, `trim`, `substring`, `indexOf`, `contains`, `startsWith`, `endsWith`, `replace`, `split`, `splitWhitespace`, `splitLines`, `isBlank`, `length`, `charAt`
- [ ] Document string interpolation: `$"Hello {name}"`
- [ ] Document escape sequences: `\n`, `\t`, `\r`, `\\`, `\"`
- [ ] Document method chaining

---

## Known Issues to Document

From analysis of ISSUES.md and code review:

1. **Function pointer calls cause SEGV** - Passing functions as parameters works but calling them fails at runtime
2. **Boolean equality operator** - `done == true` causes linker error (missing `rt_eq_bool`)
3. **Recursive lambdas not supported** - Variable not in scope during initialization
4. **String literals in interpolation** - Escaped quotes in interpolation expressions don't work (`$"Result: {f(\"hello\")}"`)

---

## Draft Specification Handling

### DATE.md (Not Implemented)

This is a well-written specification for a `Date` type. Key features planned:
- `Date.today()`, `Date.fromYmd()`, `Date.fromString()`, `Date.fromEpochDays()`
- Instance methods: `year()`, `month()`, `day()`, `weekday()`, `dayOfYear()`, etc.
- Arithmetic: `addDays()`, `addWeeks()`, `addMonths()`, `addYears()`, `diffDays()`
- Comparison: `isBefore()`, `isAfter()`, `equals()`
- Formatting: `format()`, `toIso()`, `toString()`

**Action**: Move to `docs/drafts/DATE.md` with clear "NOT IMPLEMENTED" header.

### INTEROP.md (Not Implemented)

This is a draft specification for C interoperability. Key features planned:
- `native fn` declarations for external C functions
- `#pragma include` for header inclusion
- `#pragma link` for library linking
- Type mapping between Sindarin and C

**Open Questions Documented**:
- Pointer types syntax (`*T` vs `Ptr<T>`)
- Memory ownership across Sindarin/C boundary
- Struct and opaque type support
- Variadic function support
- Safety boundary markers

**Action**: Move to `docs/drafts/INTEROP.md` with clear "DRAFT - NOT IMPLEMENTED" header.

### Format Specifiers (Potentially Unimplemented)

The `docs/format_specifiers.md` describes format specifiers like `{value:.2f}`. This may or may not be implemented.

**Action**: Verify implementation status. If not implemented, move to `docs/drafts/`.

---

## Work Items

### Phase 1: Restructure Directories

1. Create `docs/language/` directory
2. Create `docs/drafts/` directory
3. Create `docs/internal/` directory
4. Move `benchmark/BENCHMARKS.md` to `benchmark/RESULTS.md`

### Phase 2: Consolidate Language Specifications

1. **Arrays**: Verify `docs/ARRAYS.md` is complete and accurate, move to `docs/language/ARRAYS.md`, archive `ARRAY.md` notes to `docs/internal/`
2. **File I/O**: Merge valuable content from `IO.md` into `docs/FILE_IO.md`, move to `docs/language/FILE_IO.md`
3. **Memory**: Move comprehensive `MEMORY.md` to `docs/language/MEMORY.md`, remove simpler `docs/MEMORY.md`
4. **Time**: Move `TIME.md` to `docs/language/TIME.md`
5. **Strings**: Extract string documentation from `README.md` to new `docs/language/STRINGS.md`
6. **Types**: Extract types section from `README.md` to new `docs/language/TYPES.md`
7. **Overview**: Create `docs/language/OVERVIEW.md` with language philosophy and syntax

### Phase 3: Preserve Draft Specifications

1. Move `DATE.md` to `docs/drafts/DATE.md`, add "NOT IMPLEMENTED" header
2. Move `INTEROP.md` to `docs/drafts/INTEROP.md`, add "DRAFT" header
3. Review `docs/format_specifiers.md` - move to drafts if not implemented

### Phase 4: Organize Internal Documentation

1. Move `OPTIMIZATIONS.md` to `docs/internal/OPTIMIZATIONS.md`
2. Move `REFACTOR.md` to `docs/internal/REFACTOR.md`
3. Move `RUNTIME.md` to `docs/internal/RUNTIME.md`
4. Consolidate `EXPLORE.md` + test info into `docs/internal/TESTING.md`
5. Move `ISSUES.md` to `docs/internal/ISSUES.md`
6. Consolidate audit documents into `docs/internal/AUDITS.md`

### Phase 5: Update Root Documents

1. **README.md**: Trim to concise overview with links to `docs/language/`
2. **CLAUDE.md**: Update all documentation references to new paths

### Phase 6: Create Index

1. Create `docs/README.md` as documentation index with:
   - Links to all language specification documents
   - Note about draft specifications
   - Link to internal documentation for contributors

---

## Content Quality Standards

All specification documents should follow this format:

### Document Header
```markdown
# Feature Name

Brief description of the feature.

## Overview

High-level explanation with example code.
```

### Code Examples
- All code examples must be valid Sindarin syntax
- Examples should be runnable and tested
- Show expected output in comments

### Method Documentation
```markdown
### method_name(params)

Brief description.

**Parameters:**
- `param1` (type): description

**Returns:** type - description

**Example:**
```sn
// example code
```
```

### Notes and Warnings
- Use **Note:** for additional information
- Use **Warning:** for important caveats
- Document known limitations clearly

---

## Files to Delete After Consolidation

After successful consolidation and verification:

1. `ARRAY.md` (root) - Implementation notes archived, spec moved
2. `IO.md` (root) - Design decisions merged, spec moved
3. `docs/MEMORY.md` - Superseded by root version

---

## Success Criteria

1. Only `README.md` and `CLAUDE.md` remain in root (plus `DOCS.md` temporarily)
2. All language specifications are in `docs/language/` with consistent formatting
3. All draft specifications are clearly marked and in `docs/drafts/`
4. All internal documentation is in `docs/internal/`
5. `README.md` links to `docs/` for detailed information
6. All code examples in specifications have been verified against the compiler
7. No duplicate information across documents
8. Each specification accurately reflects current implementation

---

## Notes for Implementation

1. **Preserve Decision History**: When consolidating, preserve design decisions and rationale from original documents. These are valuable for understanding "why" the language works a certain way.

2. **Verify Before Moving**: Each specification should be validated against the code before being considered authoritative.

3. **Incremental Updates**: This can be done incrementally. Start with the highest-value documents (Arrays, File I/O, Memory) and work outward.

4. **Test Coverage**: Add integration tests for any undocumented but implemented features discovered during validation.

5. **Cross-References**: Ensure all documents cross-reference related documents (e.g., Memory docs should reference how File handles integrate with arenas).
