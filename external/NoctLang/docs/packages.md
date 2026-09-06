Source and bytecode modules
===========================

`require name;` asks the embedding host to resolve the logical module
name `name`.  The core VM does not choose directories, suffixes, or an
installation layout.  An embedding host enables external module
loading by setting `NoctConfig.require_resolver`; without a resolver,
a source or standalone bytecode module with requirements cannot be
loaded.

The callback returns a newly allocated exact path.  The VM takes
ownership of that string, reads the artifact, and frees the path.  It
does not call `realpath()`, normalize separators, append a suffix, or
fall back to the current directory.  The artifact may be Noct source
or a standalone bytecode module.  A resolved physical path is loaded
at most once in a VM, so a dependency shared by multiple modules is
initialized once.  Circular loads and repeated attempts to load a
failed module are errors.

## CLI resolution

The `noct` CLI supplies its own resolver.  For `require name;`,
directory precedence is:

1. the current directory;
2. directories from each `--path=DIR1:DIR2` option, in command-line order and
   in the order of components within each option.

The search is directory-major.  Within each directory, the CLI tries these
exact suffixes in order:

1. `.noct`
2. `.nct`
3. `.nbc`

Thus a `.nbc` in an earlier directory wins over source in a later
directory.  The CLI does not automatically search for legacy `.nb`
files and does not compare source and bytecode timestamps.  Module
names are ASCII-style identifiers containing letters, digits, or
underscores and may not begin with a digit.

Path lists use `:` as the separator on every platform.  A colon in an initial
Windows drive prefix such as `C:/` or `C:\\` is not a separator, and empty path
components are ignored.

For example:

```sh
noct --path=lib:vendor main.noct
noct --compile --path=lib:vendor main.noct
```

Run mode and both compile modes use the same CLI-owned search policy.
A standalone compile writes one `.nbc` for each explicit input.  Each
output contains that module's CPU bytecode and its declared
require-name list, not the dependency bodies.  Loading it later still
requires a host resolver unless its require list is empty.

## Self-contained applications

`--compile --app` packages all explicit roots and their complete transitive
closure:

```sh
noct --compile --app --path=lib:vendor program.nap main.noct
```

The resulting `.nap` contains a complete `Noct Bytecode 1.1` record
for every module, a logical require-name-to-record binding table, and
the explicit root list.  It stores no resolved physical paths.  At run
time the VM follows only the embedded bindings; it neither calls
`require_resolver` nor searches the filesystem.  A `.nap` therefore
remains runnable after its source tree, `--path` directories, and
sidecar `.nbc` files are removed.

Module discovery remains host policy.  Another embedding application
may resolve names from application resources or another store, as long
as its callback materializes the selected module as a file and returns
that file's exact allocated path.
