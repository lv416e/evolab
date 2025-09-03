# Pull Request Conventions

## PR Title Format

All pull request titles must follow the Conventional Commits specification:

```
<type>(<scope>): <description>
```

### Required Components

1. **Type**: Describes the nature of change (required)
2. **Scope**: Component or area affected (optional but recommended)
3. **Description**: Brief summary in imperative present tense (required)

## Types

| Type | Usage | Semantic Version Impact |
|------|-------|------------------------|
| `feat` | New features or enhancements | MINOR version bump |
| `fix` | Bug fixes and corrections | PATCH version bump |
| `perf` | Performance improvements | PATCH version bump |
| `refactor` | Code restructuring without functional changes | PATCH version bump |
| `test` | Adding or modifying tests | No version impact |
| `docs` | Documentation changes | No version impact |
| `style` | Code formatting, missing semicolons, etc. | No version impact |
| `build` | Build system, dependencies, tooling | No version impact |
| `ci` | CI/CD configuration changes | No version impact |
| `chore` | Maintenance tasks, updates | No version impact |

### Breaking Changes

- Add `!` after the type/scope: `feat(core)!: redesign GA interface`
- Or include `BREAKING CHANGE:` in PR body
- Results in MAJOR version bump

## Scopes

Use these standardized scopes for EvoLab components:

### Algorithm Components
- `core` - Core GA framework, concepts, base classes
- `operators` - Genetic operators (crossover, mutation, selection)
- `local-search` - Local search methods (2-opt, Lin-Kernighan)
- `problems` - Problem adapters (TSP, VRP, QAP)
- `diversity` - Diversity maintenance strategies
- `parallel` - Parallelization utilities
- `schedulers` - Adaptive operator scheduling

### Infrastructure
- `build` - CMake configuration, build system
- `ci` - GitHub Actions, testing pipeline
- `deps` - Dependencies, third-party libraries
- `docs` - Documentation, README, guides
- `tests` - Unit tests, integration tests
- `benchmarks` - Performance benchmarking
- `examples` - Example applications
- `utils` - General utilities, helpers

### Project Management
- `specs` - Kiro specifications and task management
- `config` - Configuration files, settings
- `scripts` - Utility scripts, automation

## Title Examples

### Good Examples
```
feat(operators): implement EAX crossover for TSP
fix(local-search): resolve infinite loop in 2-opt candidate list
perf(core): add candidate list optimization for large TSP instances
refactor(problems): extract distance matrix utilities
test(operators): add comprehensive EAX crossover test suite
docs(api): update genetic operator documentation
build(cmake): add TBB parallelization support
ci(actions): enable sanitizer builds for memory leak detection
```

### Poor Examples
```
❌ Update code
❌ Fix bug in operators
❌ Add new feature
❌ PR for issue #123
❌ Working on TSP improvements
❌ Final changes
```

## Title Guidelines

### Do
- Use imperative present tense ("add", "fix", "implement")
- Be specific about what changed
- Keep under 72 characters
- Reference task numbers when applicable: `feat(core): implement adaptive selection (Task 2.1)`
- Use lowercase for type and scope
- Capitalize first word of description

### Don't
- Use past tense ("added", "fixed")
- Be vague or generic
- Include issue numbers in title (use PR body instead)
- Use acronyms without context
- End with period

## Task Integration

Reference tasks from `.kiro/specs/evolab/tasks.md` in title when applicable:

```
feat(operators): implement PMX and OX crossovers (Task 1.3)
perf(parallel): add TBB integration (Task 3.1)
feat(schedulers): implement multi-armed bandit selection (Task 2.1)
```

## Automated Validation

PR titles are validated by GitHub Actions to ensure compliance:
- Type must be from approved list
- Scope must be from standardized list (if provided)
- Description must be present and properly formatted
- Length must not exceed 72 characters

## Impact on Release Process

PR titles directly drive semantic versioning:
- `feat` → Minor version bump (0.1.0 → 0.2.0)
- `fix`/`perf`/`refactor` → Patch version bump (0.1.0 → 0.1.1)
- `BREAKING CHANGE` → Major version bump (0.1.0 → 1.0.0)
- Other types → No version impact

## Branch Naming Alignment

PR titles should align with branch names:
- Branch: `feat/candidate-list-optimization`
- PR Title: `feat(local-search): add candidate list optimization for TSP`

## Examples by Task Phase

### Phase 1: Advanced Operators
```
feat(operators): implement EAX crossover for TSP (Task 1.1)
perf(local-search): add candidate list optimization (Task 1.2)
feat(operators): implement PMX and OX crossovers (Task 1.3)
```

### Phase 2: Adaptive Control
```
feat(schedulers): implement multi-armed bandit scheduler (Task 2.1)
feat(diversity): add diversity maintenance strategies (Task 2.2)
feat(core): implement restart strategies (Task 2.3)
```

### Phase 3: Parallelization
```
feat(parallel): add TBB integration for parallel evaluation (Task 3.1)
feat(parallel): implement island model architecture (Task 3.2)
perf(utils): add SIMD optimizations (Task 3.3)
```

This convention ensures consistency, enables automated tooling, and provides clear communication about the nature and impact of changes.
