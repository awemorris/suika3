/* -*- coding: utf-8; tab-width: 8; indent-tabs-mode: t; -*- */

/*
 * Noct Programming Language
 * Copyright (c) 2025, 2026, Awe Morris
 */

/*
 * GC-Unified Object Model
 */

/*
 * [Technical Whitepaper]
 * 
 *   TL;DR
 *     - Read is RMW-free for both local and shared.
 *     - Write is RMW-free for local, and locked for shared.
 *     - Value mutation is in-place, structural mutation is copy-publish.
 *     - Readers never observe destructive structural mutation.
 *     - Shared promotion is guaranteed by a safepoint barrier.
 *     - Old structural generations remain valid until the next safepoint.
 *     - GC follows a safepoint barrier and reclaims old generations naturally.
 *     - Copy-publish is unified to young copying GC in a natural manner.
 * 
 *  ============================================================================
 *   A GC-unified VM-assisted Low-complexity Read-optimized Mutable Object Model
 *  ============================================================================
 * 
 *  ----------------------------------------------------------------------------
 *   Abstract
 *  ----------------------------------------------------------------------------
 * 
 *   Modern dynamic language runtimes suffer from significant synchronization
 *   overheads when mutable objects are accessed across multiple threads. Large
 *   speculative JIT runtimes often mitigate this cost using shape invalidation,
 *   dependency tracking, and deoptimization, but such techniques are too
 *   expensive for lightweight interpreters, baseline JITs, embedded scripting
 *   runtimes, and game engine VMs.
 * 
 *   This whitepaper presents a GC-unified, VM-assisted, read-optimized mutable
 *   object model for the Noct runtime. The key design is to separate ordinary
 *   in-place value mutation from structural topology changes such as resize,
 *   rehash, and storage expansion. Value updates are performed in place, while
 *   structural mutations are handled by copy-publish through newer forwarding.
 * 
 *   As a result, shared readers never observe destructive structural mutation
 *   and do not perform reader-side Read-Modify-Write operations, epoch
 *   registration, or hazard-pointer publication. Stale structural generations
 *   remain valid until the next VM safepoint, after which the existing GC
 *   machinery can reclaim them naturally. This shifts structural lifetime
 *   coordination from a standalone concurrent data-structure protocol into the
 *   VM's existing safepoint and garbage collection infrastructure.
 * 
 *   Experimental results in the Noct runtime show that multithreaded execution
 *   degrades by only X% compared to the single-threaded baseline under the
 *   evaluated workloads.
 * 
 *  ----------------------------------------------------------------------------
 *   Introduction
 *  ----------------------------------------------------------------------------
 * 
 *   Modern dynamic language runtimes commonly represent objects, arrays, and
 *   dictionaries as mutable storage. This is convenient for language semantics,
 *   but it becomes difficult to scale when the same objects are accessed by
 *   multiple threads. Property lookup, dictionary traversal, and field access
 *   are often short operations, so even small synchronization costs on the read
 *   path can become visible in real workloads.
 * 
 *   **The Mitigations and Their Trade-offs**
 * 
 *     This scalability problem is especially important for lightweight runtimes.
 *     Large language implementations commonly rely on a complex engineering pipeline
 *     to recover performance:
 *   
 *       - Speculative JIT compilation and shape specialization.
 * 
 *       - Heavyweight dependency invalidation and deoptimization cascades.
 * 
 *     While these techniques are powerful, they require a large runtime footprint
 *     and extensive compilation machinery. Consequently, they are not always a
 *     good fit for:
 *   
 *       - Lightweight interpreters and baseline JITs.
 * 
 *       - Embedded scripting systems and game engine VMs.
 * 
 *       - Portable, AOT-oriented runtimes.
 * 
 *   **The Noct Approach**
 * 
 *     Noct takes a different approach. Instead of optimizing mutable object access
 *     through speculative compilation, it shifts the synchronization boundary of
 *     the object model itself by establishing two core invariants:
 * 
 *       1. Separation of Mutation Paths
 * 
 *          - Value Mutation: Any update that keeps the current storage layout
 *            intact. This covers both overwriting an existing slot and inserting
 *            a new entry into spare capacity. Both are performed in place on the
 *            current structural generation; no new generation is created.
 * 
 *          - Structural Mutation: An update that changes the layout itself —
 *            array resize, hash-table capacity expansion, or rehash. Only these
 *            layout-changing operations create a new structural generation and
 *            publish it through the storage's `newer` forwarding field.
 * 
 *       2. Safe Topology Invariance
 * 
 *           - Readers may observe an old structural generation, but that
 * 	    generation is never destructively modified while it may still be
 * 	    read.
 * 
 *           - Writers may update values in place, but they do not resize or
 * 	    expand the storage currently visible to readers.
 * 
 *   **VM-GC Infrastructure Integration**
 * 
 *     As a result of these invariants, the read path does not need to
 *     participate in a standalone, reader-side reclamation protocol for
 *     structural storage. Instead, the lifetime of obsolete structural
 *     generations is coordinated naturally by reusing the VM's existing global
 *     safepoint infrastructure:
 *   
 *       - Generation Lifespan: A structural generation superseded before a
 *         safepoint remains valid until that safepoint boundary.
 * 
 *       - Natural Reclamation: The garbage collector reclaims old generations safely
 *         after all managed threads have crossed the safepoint boundary.
 * 
 *   The ultimate goal of this work is not to provide a universal lock-free or
 *   wait-free data structure. The goal is to provide a small, portable, and
 *   VM-integrated object model that removes reader-side RMW operations from
 *   shared read paths while fully preserving mutable language semantics.
 * 
 *  ----------------------------------------------------------------------------
 *   Design Goals
 *  ----------------------------------------------------------------------------
 * 
 *   The primary goal of this object model is:
 * 
 *       "Reduce synchronization overhead on shared read paths while preserving
 *        a mutable object model."
 * 
 *   Dynamic language runtimes spend a vast majority of their execution cycles
 *   navigating internal collection structures:
 * 
 *       - dictionary access
 *       - property lookup
 *       - field read/write
 *       - hash table traversal
 * 
 *   In multithreaded environments, protecting these frequent operations with
 *   traditional synchronization quickly becomes the ultimate scalability
 *   bottleneck.
 * 
 *   To eliminate this bottleneck, the Noct object model targets the following
 *   features:
 * 
 *       - RMW-Free Shared Reads: Shared reads must not perform any atomic
 *         Read-Modify-Write operations.
 * 
 *       - Low Implementation Complexity: Avoid speculative JIT pipelines,
 *         complex shape tracking, or deoptimization cascades.
 * 
 *       - Target Portability: The core model must remain highly portable across
 *         everything from modern 64-bit multi-core servers to lightweight
 *         embedded systems and legacy architectures.
 * 
 *   Many concurrent data structures mitigate writer-writer conflicts but fail to
 *   alleviate reader-writer or reader-reader contention without forcing readers to
 *   participate in explicit coordination, such as:
 * 
 *       - CAS/RMW operations
 *       - epoch enter/leave
 *       - hazard pointer registration
 *       - reader tracking
 *       - version validation
 * 
 *   These costs are especially destructive for read-mostly workloads, which
 *   represent the typical pattern for property lookups and field accesses in
 *   scripting runtimes. Noct explicitly aims to reduce the cost of these shared
 *   reads to that of a raw pointer dereference followed by a consistent value
 *   load.
 * 
 *  ----------------------------------------------------------------------------
 *   Key Idea
 *  ----------------------------------------------------------------------------
 * 
 *   The central idea of the Noct object model is the strict operational
 *   decoupling of object mutations:
 * 
 *       Separate:
 *           (1) In-place value mutation
 *           (2) Structural copy-publish mutation
 * 
 *   **1. In-Place Value Mutation**
 * 
 *     Normal dictionary or array element updates are treated as mutable,
 *     in-place writes. They modify the data slots directly within the current
 *     structural generation, without allocating a new one.
 * 
 *     Crucially, inserting a new dictionary key is also an in-place operation
 *     as long as the table still has spare capacity. The new entry is written
 *     into an empty slot of the current generation; no resize and no new
 *     generation are involved.
 * 
 *     To keep the insertion atomic for concurrent readers, the entry's `key.type`
 *     tag serves as a publication word: the writer fills in the slot and then
 *     stores `key.type` with release ordering as its final step. A reader treats
 *     the key pointer as valid only after it observes `key.type == STRING`; a
 *     reader that sees an uninitialized type tag treats the slot as empty, since
 *     the insertion is not yet visible to it.
 * 
 *     Because the layout shape is invariant during any value mutation, writers
 *     only need to synchronize with other concurrent writers, leaving readers
 *     unburdened.
 * 
 *     Insertion escalates into a Structural Mutation (resize / rehash) only
 *     when the table runs out of capacity or exceeds its load factor, as
 *     described next.
 * 
 *   **2. Structural Copy-Publish Mutation**
 * 
 *     Coarser, structural topology alterations — storage (re)allocation,
 *     array resizing, hash-table capacity expansion, and rehashing — are
 *     treated as macro-operations. Instead of mutating a live table in a way
 *     that could tear or invalidate active lookups, these operations allocate
 *     an independent, entirely new structural generation and publish it
 *     atomically.
 * 
 *     Publication uses a dedicated forwarding field. Every storage header
 *     carries a single `newer` pointer:
 * 
 *       - `newer` is null while the generation is current.
 *       - When the generation is superseded, the writer installs the successor
 *         generation into `newer` with a release store. This write happens
 *         exactly once per generation and only ever transitions
 *         null -> successor.
 *       - A reader resolves the current generation by loading `newer` with
 *         acquire ordering and following it when it is non-null.
 * 
 *     Because `newer` transitions only once, a reader never observes a
 *     partially constructed successor: it sees either null (and reads the
 *     current generation) or a fully published successor. If several
 *     expansions occur within a single safepoint interval, `newer` forms a
 *     short chain that a reader follows to its tail; the chain length is
 *     bounded by the number of expansions between two safepoints and collapses
 *     when GC reclaims the old generations at the safepoint.
 * 
 *   This structural separation guarantees:
 * 
 *       Topology immutability
 *       +
 *       Value mutability
 * 
 *   at the same time, coexist harmoniously within the exact same managed object.
 * 
 *  ----------------------------------------------------------------------------
 *   Structural Copy-Publish
 *  ----------------------------------------------------------------------------
 * 
 *   Structural mutations never destructively modify the storage currently
 *   visible to readers.
 * 
 *   Readers may continue reading the old storage safely.
 * 
 *       "Readers never observe destructive structural mutation"
 * 
 *   This property allows shared reads to avoid reader-side RMW synchronization.
 * 
 *   When a structural shift is triggered, the runtime executes a non-destructive
 *   pipeline:
 * 
 *       1. Allocation: Allocate a distinct, new generation of structural storage.
 * 
 *       2. Copying: Copy old entries into the fresh layout while honoring GC rules.
 * 
 *       3. Rehashing: Rehash keys or map elements into their new slots.
 * 
 *       4. Publication: Atomically publish the new structure via the "newer"
 *          forwarding word.
 * 
 *   While this transition occurs, readers already traversing the old storage
 *   generation continue their lookups completely unaffected.
 * 
 *   Because readers never observe destructive structural mutation, they are
 *   entirely liberated from complex, reader-side RMW synchronization or state
 *   tracking.
 * 
 *   The extra allocation cost is heavily minimized by shifting the reclamation
 *   burden entirely onto the VM infrastructure.
 * 
 *  ----------------------------------------------------------------------------
 *   Shared Read Path
 *  ----------------------------------------------------------------------------
 * 
 *   Shared object reads in Noct are designed to be:
 * 
 *       - RMW-free
 * 
 *   On the read fast-path, readers do NOT:
 * 
 *       - Lock: Acquire or release heavyweight runtime locks.
 * 
 *       - Epochs: Explicitly record, increment, or modify concurrent epochs
 *         (e.g., EBR).
 * 
 *       - Hazard Pointers: Register or unregister hazard pointers to pin active
 *         layouts.
 * 
 *       - Dependency Tracking: Participate in speculative dependency tracking or
 *         deoptimization checks.
 * 
 *   Instead, a shared reader simply observes a topologically immutable
 *   generation and extracts the mutable value without RMW.
 * 
 *   **The Role of SeqLock Validation**
 * 
 *     In the current implementation, a dictionary-level sequence counter is
 *     used as a supplementary mechanism to detect and guard against torn
 *     reads. This is necessary solely because the runtime's core value type,
 *     `rt_value`, is represented as a two-machine-word structure (type tag and
 *     payload data) and cannot be read or stored as a single atomic operation
 *     across all target architectures.
 * 
 *     It is highly critical to note that **the SeqLock is an architectural
 *     artifact of the value size, not a requirement of the structural object
 *     model itself**. On hardware platforms or runtimes capable of storing
 *     values within a single naturally atomic word, this sequence validation
 *     step can be safely omitted without changing the core design.
 * 
 *  ----------------------------------------------------------------------------
 *   Shared Write Path
 *  ----------------------------------------------------------------------------
 * 
 *   Shared object writes use object-local writer locking. This is required to
 *   guarantee that there is no race write during expansion.
 * 
 *   However, normal value writes do NOT alter the storage topology or layout
 *   geometry. Because the underlying structural generation remains entirely
 *   stable during an in-place value update, readers do not require heavyweight
 *   synchronization with writers.
 * 
 *   Therefore:
 * 
 *       "Readers do not require a heavyweight synchronization with writers"
 * 
 *   At least, readers do not require RMW.
 * 
 *   Writers publish modified values using precise atomic store-release ordering
 *   rules. This ensures that any newly written pointers or data payloads are
 *   fully visible to concurrent threads before the write transaction is
 *   finalized, avoiding memory inconsistency without sacrificing reader
 *   performance.
 * 
 *  ----------------------------------------------------------------------------
 *   Safepoint-Assisted Synchronization
 *  ----------------------------------------------------------------------------
 * 
 *   The safepoint mechanism acts as a VM-wide synchronization boundary.
 * 
 *   Rather than designing a standalone concurrent data structure that requires
 *   complex thread-local tracking, Noct elegantly reuses the VM's existing
 *   global safepoint mechanism.
 * 
 *   This infrastructure, originally built to orchestrate Stop-The-World (STW)
 *   phases for garbage collection, is repurposed for three critical coordination
 *   boundaries:
 * 
 *       - Thread-Local to Shared Promotion: Ensuring that objects transitioned
 *         to the shared state are observed uniformly by all active threads.
 * 
 *       - Structural Lifetime Synchronization: Setting a deterministic boundary
 *         where old structural generations are guaranteed to be drained of all
 *         active readers.
 * 
 *       - Reclamation Boundary Coordination: Safely offloading the destruction
 *         of old tables to the GC.
 * 
 *   This approach is intentionally different from models such as:
 * 
 *       - RCU
 *       - EBR
 *       - hazard pointer systems
 * 
 *   RCU systems avoid reader locking but still force readers to actively
 *   signal their presence via epoch participation.
 * 
 *   Noct uses a technique called "VM-level Safepoint-based RCU". This completely
 *   eliminates reader-side epoch management by allowing the VM's global
 *   safepoint check to act as an implicit, zero-cost synchronization boundary
 *   for all running readers.
 * 
 *  ----------------------------------------------------------------------------
 *   Comparison with Existing Approaches
 *  ----------------------------------------------------------------------------
 * 
 *   **Biased Locking Revocation:** (Java HotSpot VM)
 * 
 *     HotSpot VM's Biased locking effectively eliminates synchronization costs
 *     as long as an object remains completely confined to its creator thread.
 * 
 *     However, once another thread attempts access, a revocation process occurs
 *     at a global safepoint, and the object permanently reverts to standard,
 *     locked synchronization paths.
 * 
 *     Noct extends this concept by:
 * 
 *       - Promoting objects to shared state
 *       - Coordinating structural mutation boundaries
 * 
 *     using the existing VM safepoint infrastructure.
 * 
 *     Noct promotes thread-local objects to shared state, but instead of
 *     returning to heavy locking, it shifts to our read-optimized, copy-publish
 *     model to maintain high performance.
 * 
 * 
 *   **RCU / EBR:**
 * 
 *     RCU-like systems avoid reader locking but still require reader-side
 *     epoch participation and reclamation coordination.
 * 
 *     More precisely, standard RCU and EBR frameworks achieve lock-free reads
 *     but shift the synchronization burden onto the reader via active epoch
 *     registration or memory fences.
 * 
 *     Noct uses a RCU variation, a technique called "VM-level Safepoint-based
 *     RCU." It avoids reader-side epoch tracking entirely by leveraging the
 *     pre-existing, obligatory safepoint checks that managed threads already
 *     execute for cooperative multitasking and GC.
 * 
 *   **Speculative Runtimes:** (e.g., TruffleRuby / V8)
 * 
 *     High-performance speculative JIT runtimes often rely on:
 * 
 *       - Assumptions on global object shapes and dependency graphs
 *       - Shape invalidation
 *       - Deoptimization
 * 
 *     to optimize shared reads. For example, when an object structure changes, a
 *     deoptimization cascade travels up the call stack to invalidate compiled
 *     methods.
 * 
 *     Noct intentionally avoids:
 * 
 *       - Speculative optimization
 * 
 *       - Dependency invalidation
 * 
 *       - Deoptimization cascades
 * 
 *     because these are extremely complex in compiler design.
 * 
 *     Noct prioritizes:
 * 
 *       - Smaller footprint and simpler runtime architecture suitable for
 *         embedded systems.
 * 
 *       - Deterministic execution.
 * 
 *       - Smaller latency.
 * 
 *  ----------------------------------------------------------------------------
 *   Young Copy GC Integration
 *  ----------------------------------------------------------------------------
 * 
 *   The structural copy-publish strategy is designed together with the GC
 *   architecture, rather than as a standalone concurrent hash table algorithm.
 *   It integrates seamlessly with a generational young copy GC.
 * 
 *   When a dictionary or array is expanded, its old structural generation is
 *   left completely intact for readers. Because its lifetime is guaranteed
 *   to be bounded by the very next safepoint boundary, the old storage naturally
 *   acts as an ephemeral young object.
 * 
 *   Once the VM strikes a safepoint to execute a young collection, the
 *   scavenging phase can instantly reclaim the stale generation without any
 *   extra overhead or manual reference counting.
 * 
 *   By unifying structural copy-publish with the GC architecture, Noct avoids
 *   the classic memory fragmentation and tracking overhead that plagues
 *   standalone concurrent hash maps.
 * 
 *  ----------------------------------------------------------------------------
 *   Storage State Transitions
 *  ----------------------------------------------------------------------------
 * 
 *   To maintain deterministic paths, array and dictionary storage balances two
 *   orthogonal properties:
 * 
 *       (1) Ownership State: Dictates synchronization requirements
 * 
 *           Thread-Local
 *           vs.
 *           Shared
 * 
 *       (2) Structural Generation: Tracks topology mutations
 * 
 *           Current Storage
 *           vs.
 *           Old Stale Storage, superseded by newer forwarding
 * 
 *   These states are intentionally separated and kept completely independent.
 * 
 *   In-place value mutations — overwriting an existing slot, or inserting a
 *   new entry into spare capacity — update the data payload but never change
 *   the structural generation.
 * 
 *   Only geometric modifications such as resize, expansion, or rehash force a
 *   change in generation, ensuring that structural overhead is paid
 *   exclusively when the layout itself changes.
 * 
 *  ----------------------------------------------------------------------------
 *   Thread-Local Storage
 *  ----------------------------------------------------------------------------
 * 
 *   Every newly allocated array or dictionary begins life as a:
 * 
 *       - Thread-local storage
 * 
 *   While the array or dictionary is accessed only by its creator thread,
 *   the runtime bypasses all concurrent machinery via a high-speed fast path:
 * 
 *       - Reads entirely skip RMW or memory fences.
 * 
 *       - Writes skip the object-local write lock and require no RMW.
 * 
 *       - Layout expansion is handled directly by the creator thread without
 *         coordination.
 * 
 *   Each storage carries a `shared` flag that selects its access path. While the
 *   flag is clear (`!shared`), every access first compares the storage's `creator`
 *   field against the accessing thread's handle:
 * 
 *       - If they match, the access belongs to the creator and proceeds on the
 *         thread-local fast path.
 * 
 *       - If they differ, a foreign thread has touched the storage, and it must
 *         be promoted to the shared state.
 * 
 *   This promotion is executed at a safepoint boundary managed by the STW
 *   executor, so all threads observe the transition — and the newly set `shared`
 *   flag — uniformly. After promotion, the flag alone routes every access to the
 *   shared protocol, and the per-access creator comparison is no longer needed.
 * 
 *       thread-local storage
 *           --> safepoint
 *           --> shared promotion
 *           --> shared storage
 * 
 *   After promotion, all threads observe the array as shared.
 * 
 *  ----------------------------------------------------------------------------
 *   Thread-Local Expansion
 *  ----------------------------------------------------------------------------
 * 
 *   If the creator thread expands a thread-local array or dictionary, the old
 *   storage is not destructively resized. Instead, the creator thread allocates
 *   a fresh generation, copies the elements, and links it via newer forwarding.
 * 
 *       Thread-local storage
 *           -> [Expand Triggered]
 * 	  -> Thread-Local Newer Allocated
 *           -> Old Generation Remains Readable
 *           -> Safepoint Boundary Crossed
 *           -> GC collects old storage
 * 
 *   By enforcing the exact same copy-publish lifecycle for both thread-local and
 *   shared expansions, the runtime preserves a single, unified structural
 *   invariant.
 * 
 *   The old storage lifetime is bounded by the next safepoint. This keeps the
 *   rule identical to shared expansion and avoids a separate reclamation
 *   protocol.
 * 
 *  ----------------------------------------------------------------------------
 *   Shared Storage
 *  ----------------------------------------------------------------------------
 * 
 *   Once promoted to shared status, the storage shifts to a read-optimized
 *   concurrent protocol.
 * 
 *     Shared reads:
 * 
 *       - do not acquire the write lock
 *       - do not perform reader-side RMW operations
 *       - follow newer forwarding before the read begins
 * 
 *   Shared writers serialize by acquiring the object-local lock, performing
 *   the in-place value write.
 * 
 *     Shared writes:
 * 
 *       - acquire the object-local write lock
 *       - update the sequence counter to mark the value unstable
 *       - perform the in-place value write
 *       - update the sequence counter again to mark the value stable
 * 
 *   Therefore, normal shared writes mutate values in place but do not mutate the
 *   storage topology.
 * 
 *  ----------------------------------------------------------------------------
 *   Shared Expansion
 *  ----------------------------------------------------------------------------
 * 
 *   When a shared array or dictionary requires expansion or rehashing, it performs
 *   a non-destructive structural mutation using our copy-publish pipeline:
 * 
 *       Shared Current -> [Acquire Write Lock]
 *                      -> [Allocate Newer Storage]
 *                      -> [Copy & Rehash]
 *                      -> [Publish Newer via Release Store]
 *                      -> [Release Write Lock]
 *                      -> [Old Storage Remains Safely Readable by Pre-Boundary Readers]
 *                      -> [Safepoint Boundary Reached]
 *                      -> [Stale Storage Naturally Reclaimed by GC]
 * 
 *   Readers that already started before publication may continue reading the old
 *   storage until their next safepoint.
 * 
 *   Readers that start after publication follow newer forwarding and read the
 *   newer storage.
 * 
 *   The old storage is never reclaimed immediately after publication. Its
 *   lifetime is synchronized by the VM safepoint mechanism and GC.
 * 
 *  ----------------------------------------------------------------------------
 *   GC Entry and Structural Lifetime
 *  ----------------------------------------------------------------------------
 * 
 *   Entering GC is also a safepoint synchronization boundary for the entire
 *   runtime.
 * 
 *   Before the actual GC mechanics execute, a strict synchronization handshake
 *   takes place:
 * 
 *       - the executing thread becomes non-inflight
 *       - one thread is promoted to the STW executor
 *       - all other threads must become non-inflight
 * 
 *   Once all threads are non-inflight, no thread can still be reading an old
 *   storage generation from before the safepoint.
 * 
 *   Additionally, the runtime enforces a re-acquisition invariant: a managed
 *   thread may never cache a raw storage pointer across a safepoint. After any
 *   safepoint, before touching the object again, a thread must re-load the
 *   storage pointer from the object header and re-resolve `newer`. This is an
 *   internal rule maintained entirely by the runtime; script-level code never
 *   observes or manages it, and the mutable language semantics seen by the
 *   programmer are unchanged.
 * 
 *   Consequently, it is guaranteed that no thread can still be reading or
 *   holding a pointer to an old structural generation superseded prior to the
 *   safepoint: any pointer held before the safepoint has been discarded, and
 *   any pointer acquired afterward necessarily resolves to the current
 *   generation. Storage generations superseded via `newer` before the
 *   safepoint can therefore be treated as unreachable and reclaimed by GC.
 * 
 *   In short:
 * 
 *       - newer forwarding publishes the next structural generation
 *       - safepoint ends the lifetime of pre-boundary readers
 *       - GC reclaims old structural generations
 * 
 *  ----------------------------------------------------------------------------
 *   Summary
 *  ----------------------------------------------------------------------------
 * 
 *   The complete, unified state transitions of the Noct object model can be
 *   summarized as:
 * 
 *       Thread-Local Storage
 *           -> [Safepoint Promotion]
 *           -> Shared Storage
 * 
 *       Thread-Local Storage
 *           -> [Creator Expansion]
 * 	  -> Old + Newer Storage
 * 	  -> [Safepoint]
 * 	  -> GC Collects Old
 * 
 *       Shared Storage
 *           -> [Locked Expansion]
 * 	  -> Old + Newer Storage
 * 	  -> [Safepoint]
 * 	  -> GC Collects Old
 * 
 *   The core invariants that must be preserved at all times are:
 * 
 *       - Structural mutation is copy-publish.
 * 
 *       - Value mutation is in-place.
 * 
 *       - Stale structural generations survive until the next safepoint.
 * 
 *   Synchronization responsibility table:
 * 
 *    Mechanism              Protects / guarantees
 *    --------------------------------------------------------------------------
 *    write_lock             Excludes concurrent writers.
 * 
 *    newer forwarding       Publishes a new structural generation.
 *                           Prevents destructive resize/rehash visibility.
 * 
 *    safepoint              Ends the lifetime of pre-boundary readers.
 *                           Allows GC to reclaim old structural generations.
 * 
 *    STW executor           Serializes GC, shared promotion, and structural
 *                           lifetime-boundary processing.
 * 
 *    shared flag            Selects thread-local fast path vs shared protocol.
 * 
 *    dict key.type          Publication word for dictionary entry insertion.
 *                           Readers may inspect key pointer only after seeing
 *                           key.type == STRING.
 * 
 *  ----------------------------------------------------------------------------
 *   Design Philosophy
 *  ----------------------------------------------------------------------------
 * 
 *   This object model is explicitly NOT intended to be:
 * 
 *       - universally lock-free
 *       - wait-free
 *       - maximally scalable at any complexity cost
 * 
 * 
 *   Instead, the design prioritizes:
 * 
 *       - low implementation complexity
 * 
 *       - predictable latency
 * 
 *       - low runtime footprint
 * 
 *       - portability
 * 
 *       - VM/GC integration
 * 
 *       - read-mostly performance
 * 
 *   The goal is to provide a practical synchronization model suitable for
 *   lightweight dynamic language runtimes that maximizes read performance while
 *   preserving native mutable scripting semantics.
 * 
 *  ----------------------------------------------------------------------------
 *   Evaluation (to be evaluated)
 *  ----------------------------------------------------------------------------
 * 
 *   We evaluate the performance of the proposed object model using
 *   microbenchmarks and real-world game engine workloads on modern and classic
 *   SMP architectures (Intel Lunar Lake, Apple M5, and Apple PowerPC G5)
 * 
 *   1. Read-Mostly Scalability (Microbenchmark)
 * 
 *    - Measure property lookup throughput with varying thread counts (1 to 16)
 *      under a 99% read / 1% write workload.
 * 
 *    - Expected result: Near-linear scalability due to RMW-free shared reads.
 * 
 *   2. Write Performance
 * 
 *    - Compare in-place shared writes against standard mutex-locked dict writes.
 * 
 *    - Expected result: Shared writes incur slightly higher latency due to 
 *      write lock, but prevent reader degradation.
 * 
 *   3. Structural Copy-Publish and GC Pause Times
 * 
 *    - Evaluate the impact of thread-local and shared dictionary expansion 
 *      on young copy GC pause times.
 * 
 *    - Expected result: Minimal impact on STW duration since old generation
 *      reclamation is seamlessly integrated into the scavenge phase.
 * 
 *   4. Real-World Engine Workload (Playfield Engine Benchmark)
 * 
 *    - Measure frame rate stability and script execution throughput under
 *      heavy multithreaded background resource loading.
 * 
 *    - Result: Multithreaded execution exhibits only a X% performance 
 *      degradation compared to the single-threaded baseline mode.
 * 
 *  ----------------------------------------------------------------------------
 *   Future Work
 *  ----------------------------------------------------------------------------
 * 
 *    1. Shared to Thread-Local Demotion
 *     - Objects that are initially promoted to shared state but subsequently
 *       accessed only by a single thread can be demoted back to thread-local
 *       state. This transition can be safely coordinated at a young GC safepoint
 *       boundary after a specific survival threshold.
 * 
 *    2. Dynamic Rehash Threshold Tuning
 *     - Optimize the 75% load factor threshold for dictionary expansion based on
 *       runtime hardware cache miss counters (PMC coordination).
 * 
 *    3. Architecture-Specific Atomic Optimizations
 *     - Utilize AArch64 'LDAPR' (Load-Acquire RCpc) instructions to further
 *       optimize the shared read path on weakly-ordered hardware.
 * 
 *  ----------------------------------------------------------------------------
 *   Conclusion
 *  ----------------------------------------------------------------------------
 * 
 *   In this whitepaper, we proposed a mutable object model that eliminates
 *   reader-side Read-Modify-Write (RMW) synchronization by unifying structural
 *   lifetime coordination with the existing VM safepoint and GC infrastructure.
 *   By decoupling in-place value mutation from structural copy-publish
 *   expansion, the Noct runtime achieves deterministic and highly scalable
 *   shared reads without the architectural complexity of speculative JIT
 *   invalidations or heavyweight hazard pointer tracking.
 */

#include <noct/noct.h>
#include "objectmodel.h"
#include "runtime.h"
#include "gc.h"
#include "atomic.h"

#include <assert.h>

/* False Assertion */
#define NO_SPACE_FOR_DICTIONARY		(0)

/*
 * ============================================================================
 *  STW Request Helpers
 * ============================================================================
 *
 *  STW request invariant:
 *
 *   - A thread raises stw_request_counter before trying to become the
 *     STW executor.
 *
 *   - Once raised, the request is not lowered merely because another
 *     thread is currently the executor.
 *
 *   - The request is lowered only by the thread itself, after it has
 *     successfully become the STW executor and completed its STW section.
 *
 *   - Therefore, stw_request_counter == 0 means that all raised STW
 *     requests have completed their executor sections.
 */

/*
 * Helper: Raise a STW request for the current thread.
 */
static INLINE void
raise_stw_request(
	struct rt_env *env)
{
	assert(!env->is_stw_raised);

	/*
	 * Raise a STW request.
	 */
	atomic_fetch_add_acquire_int(&env->vm->stw_request_counter, 1);

	env->is_stw_raised = true;
}
	
/*
 * Helper: Lower the STW request for the current thread.
 */
static INLINE void
lower_stw_request(
	struct rt_env *env)
{
	assert(env->is_stw_raised);

	/*
	 * Lower the STW request.
	 */
	atomic_fetch_sub_release_int(&env->vm->stw_request_counter, 1);

	env->is_stw_raised = false;
}

/*
 * Helper: Check if there are STW requests raised.
 */
static INLINE bool
check_if_stw_requests_raised(
	struct rt_env *env)
{
	/*
	 * Check the stw_request_counter.
	 */
	if (atomic_load_acquire_int(&env->vm->stw_request_counter) == 0) {
		/*
		 * No requests.
		 */
		return false;
	}

	/*
	 * One or more requests exist.
	 */
	return true;
}

/*
 * Helper: Wait until all raised STW requests are completed.
 */
static INLINE void
ensure_all_stw_requests_completed(
	struct rt_env *env)
{	
	uint64_t cpu_relax_base = CPU_RELAX_BASE_INITIALIZER;

	/*
	 * This function is called by a non-STW-executor thread that has already
	 * entered a safepoint. It does not wait merely for the current executor
	 * to disappear.  Instead, it waits until every raised STW request has
	 * been processed by some STW executor.
	 *
	 * A raised STW request is never withdrawn just because another executor
	 * exists.  The request is lowered only after the requesting thread has
	 * successfully become an STW executor and completed its STW section.
	 *
	 * Therefore:
	 *
	 *      stw_request_counter == 0
	 *
	 * means:
	 *
	 *      all STW requests raised before or during this wait have
	 *      completed their STW executor sections.
	 *
	 * This is intentionally stronger than waiting for:
	 *
	 *      stw_executor_counter == 0
	 *
	 * because multiple threads may have raised independent STW requests.
	 */
	assert(!env->is_in_flight);
	assert(!env->is_stw_executor);

	/*
	 * Loop while there are STW requests.
	 */
	while (check_if_stw_requests_raised(env)) {
		/*
		 * Yield or sleep.
		 */
		cpu_relax(&cpu_relax_base);
	}
}

/*
 * ============================================================================
 *  Safepoint Helpers
 * ============================================================================
 *
 *  in_flight_counter invariant:
 *
 *   - in_flight_counter counts mutator threads that are outside safepoints.
 *   - enter_safepoint() decrements it exactly once.
 *   - leave_safepoint() increments it exactly once.
 *   - env->is_safepoint prevents double-enter and double-leave.
 *
 *  Therefore, when the STW-executor observes in_flight_counter == 0, all
 *  mutator threads are parked at safepoints and no pre-boundary reader is still
 *  executing.
 */

/*
 * Helper: Set the thread non-inflight and enter a safepoint.
 */
static INLINE void
enter_safepoint(
	  struct rt_env *env)
{
	assert(env->is_in_flight);

	/*
	 * This have to be a release-store because a promoted STW-executor does
	 * acquire-load.
	 */
	atomic_fetch_sub_release_int(&env->vm->in_flight_counter, 1);

	/*
	 * Rememer the state. Note that env is thread-local context and we don't
	 * need atomic release store.
	 */
	env->is_in_flight = false;
}

/*
 * Helper: Set the thread non-inflight ane leave the safepoint.
 */
static INLINE void
leave_safepoint(
	  struct rt_env *env)
{
	assert(!env->is_in_flight);

	/*
	 * This have to be a release-store because a promoted STW-executor does
	 * acquire-load.
	 */
	atomic_fetch_add_release_int(&env->vm->in_flight_counter, 1);

	/*
	 * Rememer the state. Note that env is thread-local context and we don't
	 * need atomic release store.
	 */
	env->is_in_flight = true;
}

/*
 * Helper: Wait for all other threads to reach their safepoints.
 */
static INLINE void
ensure_all_threads_reach_safepoints(
	struct rt_env *env)
{
	uint64_t cpu_relax_base = CPU_RELAX_BASE_INITIALIZER;

	assert(!env->is_in_flight);
	assert(env->is_stw_executor);

	/*
	 * Loop while there are in-flight threads. Note that the current thread
	 * is in safepoint and is the STW-executor.
	 */
	while (atomic_load_acquire_int(&env->vm->in_flight_counter) > 0) {
		/*
		 * Yield or sleep, according to the elapsed time.
		 */
		cpu_relax(&cpu_relax_base);
	}
}

/*
 * Helper: Synchronize on a safepoint.
 */
static INLINE void
synchronize_safepoints(
	  struct rt_env *env)
{
	assert(env->is_in_flight);
	assert(!env->is_stw_executor);

retry:
	/*
	 * Check for raised STW requests from other threads.
	 */
	if (!check_if_stw_requests_raised(env)) {
		/*
		 * No STW request exists and no need for synchronization. Move
		 * forward without waiting for other threads.
		 */
		return;
	}

	/*
	 * Enter a safepoint.
	 */
	enter_safepoint(env);

	/*
	 * Wait for other thread to complete STW-executor process. Note that the
	 * STW-executor proecss includes GC, shared-promotion, or structural
	 * boundary.
	 */
	ensure_all_stw_requests_completed(env);

	/*
	 * Leave the safepoint.
	 */
	leave_safepoint(env);

	/*
	 * We have to go back and check the STW requests again.
	 */
	goto retry;
}

/*
 * ============================================================================
 *  STW Executor Promotion Helpers
 * ============================================================================
 *
 *  STW-executor invariant:
 *
 *   - A thread must enter a safepoint before becoming the STW-executor.
 *   - The STW-executor itself is non-inflight while executing the STW section.
 *   - The STW-executor may execute GC, shared promotion, or structural lifetime
 *     boundary processing.
 *   - A thread must demote from the STW-executor before leaving the safepoint.
 */

/*
 * Helper: Try promoting to the STW-executor.
 */
static INLINE bool
try_promote_to_stw_executor(
	struct rt_env *env)
{
	int old;

	assert(!env->is_stw_executor);

	old = atomic_fetch_add_acquire_int(&env->vm->stw_executor_lock, 1);
	if (old == 0) {
		/*
		 * Succeeded. 
		 */
		env->is_stw_executor = true;
		return true;
	}

	atomic_fetch_sub_release_int(&env->vm->stw_executor_lock, 1);

	return false;
}
	
/*
 * Helper: Wait for the current STW executor to be demoted.
 */
static INLINE void
wait_for_stw_executor_demotion(
	struct rt_env *env)
{
	uint64_t cpu_relax_base = CPU_RELAX_BASE_INITIALIZER;

	assert(!env->is_stw_executor);

	while (atomic_load_acquire_int(&env->vm->stw_executor_lock) > 0)
		cpu_relax(&cpu_relax_base);
}

/*
 * Helper: Promote to the STW-executor.
 */
static INLINE void
promote_to_stw_executor(
	struct rt_env *env)
{
	assert(!env->is_in_flight);
	assert(!env->is_stw_executor);
 
	/*
	 * Raise a STW request.
	 */
	raise_stw_request(env);
	
retry:
	/*
	 * Try promotion.
	 */
	if (try_promote_to_stw_executor(env)) {
		/*
		 * Successfully promoted to the STW-executor. Wait until all
		 * other threads enter their safepoints.
		 */
		ensure_all_threads_reach_safepoints(env);

		/*
		 * Succeeded. Now this thread is the STW-executor.
		 */
		return;
	}

	/*
	 * Failed: Another thread has promoted to the STW-executor. Wait for the
	 * thread to demote from the STW-executor.
	 */
	wait_for_stw_executor_demotion(env);

	/*
	 * Go back and restart.
	 */
	goto retry;
}

/*
 * Helper: Demote from the STW executor.
 */
static INLINE void
demote_from_stw_executor(
	struct rt_env *env)
{
	assert(env->is_stw_executor);

	/*
	 * Decrement stw_executor_counter.
	 */
	atomic_fetch_sub_release_int(&env->vm->stw_executor_lock, 1);

	/*
	 * Lower the STW request.
	 */
	lower_stw_request(env);

	env->is_stw_executor = false;
}

/*
 * ============================================================================
 *  Write Lock Helpers
 * ============================================================================
 */

/*
 * Helper: Try a write lock for an array.
 */
static INLINE bool
add_try_array_write_lock(
	struct rt_env *env,
	struct rt_array *arr)
{
	int old;

	UNUSED_PARAMETER(env);

	old = atomic_fetch_add_acquire_int(&arr->write_lock, 1);
	if (old > 0) {
		/*
		 * Failed to lock.
		 */
		atomic_fetch_sub_release_int(&arr->write_lock, 1);
		return false;
	}

	/*
	 * Successfully locked.
	 */
	return true;
}

/*
 * Helper: Try a write lock for a dictionary.
 */
static INLINE bool
add_try_dict_write_lock(
	struct rt_env *env,
	struct rt_dict *dict)
{
	int old;

	UNUSED_PARAMETER(env);

	old = atomic_fetch_add_acquire_int(&dict->write_lock, 1);
	if (old > 0) {
		/*
		 * Failed to lock.
		 */
		atomic_fetch_sub_release_int(&dict->write_lock, 1);
		return false;
	}

	/*
	 * Successfully locked.
	 */
	return true;
}

/*
 * Helper: Do a write unlock for an array.
 */
static INLINE void
sub_array_write_unlock(
	struct rt_env *env,
	struct rt_array *arr)
{
	UNUSED_PARAMETER(env);

	atomic_fetch_sub_release_int(&arr->write_lock, 1);
}

/*
 * Helper: Do a write unlock for a dictionary.
 */
static INLINE void
sub_dict_write_unlock(
	struct rt_env *env,
	struct rt_dict *dict)
{
	UNUSED_PARAMETER(env);

	atomic_fetch_sub_release_int(&dict->write_lock, 1);
}

/*
 * ============================================================================
 *  SeqLock Helpers
 * ============================================================================
 *
 *  SeqLock invariant:
 *
 *   - SeqLock protects only multi-word rt_value load/store consistency.
 *   - SeqLock does not protect structural mutation.
 *   - Structural mutation is protected by copy-publish and safepoint lifetime.
 *   - Writers must hold the object write lock while updating the SeqLock.
 *   - Readers must not acquire the write lock. They only retry when the SeqLock
 *     changes or is odd.
 */

/*
 * Helper: Increment the SeqLock in an array.
 */
static INLINE void
increment_array_seqlock(
	struct rt_env *env,
	struct rt_array *arr)
{
	UNUSED_PARAMETER(env);

	atomic_fetch_add_release_int(&arr->seqlock, 1);
}

/*
 * Helper: Increment the SeqLock in a dictionary.
 */
static INLINE void
increment_dict_seqlock(
	struct rt_env *env,
	struct rt_dict *dict)
{
	UNUSED_PARAMETER(env);

	atomic_fetch_add_release_int(&dict->seqlock, 1);
}

/*
 * Helper: Get the SeqLock in an array.
 */
static INLINE int
get_array_seqlock(
	struct rt_env *env,
	struct rt_array *arr)
{
	int seq;

	UNUSED_PARAMETER(env);

	seq = atomic_load_acquire_int(&arr->seqlock);

	return seq;
}

/*
 * Helper: Get the SeqLock in a dictionary.
 */
static INLINE int
get_dict_seqlock(
	struct rt_env *env,
	struct rt_dict *dict)
{
	int seq;

	UNUSED_PARAMETER(env);

	seq = atomic_load_acquire_int(&dict->seqlock);

	return seq;
}

/*
 * ============================================================================
 *  GC In-Progress Count Helpers
 * ============================================================================
 */

/*
 * Helper: Increment the GC-in-progress count.
 */
static INLINE void
increment_gc_in_progress_count(
	struct rt_env *env)
{
	UNUSED_PARAMETER(env);

	atomic_fetch_add_release_int(&env->vm->gc_in_progress_counter, 1);
}

/*
 * Helper: Decrement the GC-in-progress count.
 */
static INLINE void
decrement_gc_in_progress_count(
	struct rt_env *env)
{
	UNUSED_PARAMETER(env);

	atomic_fetch_sub_release_int(&env->vm->gc_in_progress_counter, 1);
}

/*
 * Helper: Check if a nested GC.
 */
static INLINE bool
check_if_nested_gc(
	struct rt_env *env)
{
	if (atomic_load_acquire_int(&env->vm->gc_in_progress_counter) > 0){
		/*
		 * Nested.
		 */
		return true;
	}

	/*
	 * Not nested.
	 */
	return false;
}

/*
 * ============================================================================
 *  Object Pointer Acquisition Helpers
 * ============================================================================
 *
 *  Object pointer acquisition invariant:
 *
 *   - A normal mutator may follow object pointers only while it is in-flight
 *     (in-flight = not in a safepoint).
 *   - A thread in a safepoint may follow object pointers only if it is the 
 *     STW-executor.
 *
 *   This is required because a non-executor safepoint thread is logically
 *   parked. It must not observe or retain object pointers while GC or
 *   structural boundary processing may be in progress.
 *
 *  Newer forwarding invariant:
 *
 *   - newer is written exactly once.
 *   - newer is published with release ordering after the new storage has been
 *     fully initialized.
 *   - Readers follow newer with acquire ordering before starting a read.
 *   - Old storage remains valid until a safepoint boundary.
 *   - GC may reclaim old storage only after no pre-boundary reader can still
 *     hold it.
 *
 *  Multiple newer hops are allowed.
 *
 *   A reader always follows the chain to the latest storage before starting the
 *   operation. A writer must re-follow the chain after any allocation that may
 *   trigger GC or safepoint synchronization.
 */

/*
 * Helper: Get the current array pointer.
 */
static INLINE struct rt_array *
follow_array_pointer(
	struct rt_env *env,
	struct rt_value *arr_val)
{
	struct rt_array *arr, *next;

	assert(env != NULL);
	assert(arr_val != NULL);
	assert(arr_val->type == NOCT_VALUE_ARRAY);
	assert(env->is_in_flight || (!env->is_in_flight && env->is_stw_executor));

	/*
	 * Read-acquire "arr_val->val.arr" because the array may be moved by the
	 * Copy-GC or Compaction-GC in another thread during the previous
	 * safepoint.
	 */
	arr = atomic_load_acquire_ptr((void**)&arr_val->val.arr);

	/*
	 * Track the newer forwarding chain. Note that array/dictionary
	 * expansion is done by creating a new array/dictionary and linking it
	 * to the newer field in the old array/dictionary. This mechanism is
	 * called copy-publish, enabling the structural topology immutable.
	 */
	while ((next = atomic_load_acquire_ptr((void **)&arr->newer)) != NULL)
		arr = next;

	return arr;
}

/*
 * Helper: Get the current dictionary pointer.
 */
static INLINE struct rt_dict *
follow_dict_pointer(
	struct rt_env *env,
	struct rt_value *dict_val)
{
	struct rt_dict *dict, *next;

	assert(env != NULL);
	assert(dict_val != NULL);
	assert(dict_val->type == NOCT_VALUE_DICT);

	/*
	 * Read-acquire "dict_val->val.dict" because the array may be moved by
	 * the Copy-GC or Compaction-GC in another thread during the previous
	 * safepoint.
	 */
	dict = atomic_load_acquire_ptr((void**)&dict_val->val.dict);

	/*
	 * Track the newer forwarding chain. Note that array/dictionary
	 * expansion is done by creating a new array/dictionary and linking it
	 * to the newer field in the old array/dictionary. This mechanism is
	 * called copy-publish, enabling the structural topology immutable.
	 */
	while ((next = atomic_load_acquire_ptr((void **)&dict->newer)) != NULL)
		dict = next;

	return dict;
}

/*
 * ============================================================================
 *  Shared Flag and Promotion Helpers
 * ============================================================================
 *
 *  Shared promotion invariant:
 *
 *   - Promotion changes only the ownership state, not the structural generation.
 *   - Promotion must happen at a safepoint boundary by the STW-executor.
 *   - After the shared flag is published, no thread may use the thread-local
 *     fast path for that storage. This is ensured by safepoint synchronization.
 */

/*
 * Helper: Set the shared flag of an array and publish.
 */
static INLINE void
publish_array_shared(
	struct rt_env *env,
	struct rt_array *arr)
{
	assert(env->is_stw_executor);
	assert(!arr->shared);

	/*
	 * Publish the shared flag.
	 */
	atomic_store_release_int(&arr->shared, 1);
}

/*
 * Helper: Set the shared flag of a dictionary and publish.
 */
static INLINE void
publish_dict_shared(
	struct rt_env *env,
	struct rt_dict *dict)
{
	assert(env->is_stw_executor);
	assert(!dict->shared);

	/*
	 * Publish the shared flag.
	 */
	atomic_store_release_int(&dict->shared, 1);
}

/*
 * Helper: Promote an array to shared.
 */
static INLINE void
promote_array_to_shared(
	struct rt_env *env,
	struct rt_value *arr_val)
{
	struct rt_array *arr;

	/*
	 * Enter a safepoint.
	 */
	enter_safepoint(env);

	/*
	 * Promote to the STW-executor.
	 */
	promote_to_stw_executor(env);

	/*
	 * Get the array pointer.
	 */
	arr = follow_array_pointer(env, arr_val);

	/*
	 * Publish the shared flag.
	 */
	publish_array_shared(env, arr);

	/*
	 * Ensure the boundary by waiting until all threads reach safepoints.
	 */
	ensure_all_threads_reach_safepoints(env);

	/*
	 * Demote from the STW-executor.
	 */
	demote_from_stw_executor(env);

	/*
	 * Leave the safepoint.
	 */
	leave_safepoint(env);
}

/*
 * Helper: Promote a dictionary to shared.
 */
static INLINE void
promote_dict_to_shared(
	struct rt_env *env,
	struct rt_value *dict_val)
{
	struct rt_dict *dict;

	/*
	 * Enter a safepoint.
	 */
	enter_safepoint(env);

	/*
	 * Promote to the STW-executor.
	 */
	promote_to_stw_executor(env);

	/*
	 * Get the array pointer.
	 */
	dict = follow_dict_pointer(env, dict_val);

	/*
	 * Publish the shared flag.
	 */
	publish_dict_shared(env, dict);

	/*
	 * Ensure the boundary by waiting until all threads reach safepoints.
	 */
	ensure_all_threads_reach_safepoints(env);

	/*
	 * Demote from the STW-executor.
	 */
	demote_from_stw_executor(env);

	/*
	 * Leave the safepoint.
	 */
	leave_safepoint(env);
}

/*
 * ============================================================================
 *  Read Access Guarantee Helpers
 * ============================================================================
 */

/*
 * Helper: Start array read.
 */
static INLINE struct rt_array *
start_array_read(
	struct rt_env *env,
	struct rt_value *arr_val)
{
	struct rt_array *arr;

retry:
	/*
	 * Synchronize to safepoints.
	 */
	synchronize_safepoints(env);

	/*
	 * Get the array pointer.
	 */
	arr = follow_array_pointer(env, arr_val);

	/*
	 * In case the array is thread-local, and this thread is not the creator
	 * of the array.
	 */
	if (!arr->shared && env != arr->creator) {
		/*
		 * Promote the array to shared.
		 */
		promote_array_to_shared(env, arr_val);

		/*
		 * Restart from the beginning.
		 */
		goto retry;
	}

	/*
	 * Start a read. The array pointer will be valid until the next
	 * safepoint, even if a newer structural is published.
	 */
	return arr;
}

/*
 * Helper: Start dictionary read.
 */
static INLINE struct rt_dict *
start_dict_read(
	struct rt_env *env,
	struct rt_value *dict_val)
{
	struct rt_dict *dict;

retry:
	/*
	 * Synchronize to safepoints.
	 */
	synchronize_safepoints(env);

	/*
	 * Get the dictionary pointer.
	 */
	dict = follow_dict_pointer(env, dict_val);

	/*
	 * In case the dictionary is thread-local, and this thread is not the
	 * creator of the dictionary.
	 */
	if (!dict->shared && env != dict->creator) {
		/*
		 * Promote the dictionary to shared.
		 */
		promote_dict_to_shared(env, dict_val);

		/*
		 * Restart from the beginning.
		 */
		goto retry;
	}

	/*
	 * Start a read. The dictionary pointer will be valid until the next
	 * safepoint, even if a newer structural is published.
	 */
	return dict;
}

/*
 * Helper: Start two dictionary read.
 */
static INLINE void
start_dict_read2(
	struct rt_env *env,
	struct rt_value *dict1_val,
	struct rt_value *dict2_val,
	struct rt_dict **dict1,
	struct rt_dict **dict2)
{
	struct rt_dict *d1, *d2;

retry:
	/*
	 * Synchronize to safepoints.
	 */
	synchronize_safepoints(env);

	/*
	 * Get the dictionary pointer for dict1_val.
	 */
	d1 = follow_dict_pointer(env, dict1_val);

	/*
	 * In case the dict1 is thread-local, and this thread is not the
	 * creator of the dictionary.
	 */
	if (!d1->shared && env != d1->creator) {
		/*
		 * Promote the dictionary to shared.
		 */
		promote_dict_to_shared(env, dict1_val);

		/*
		 * Restart from the beginning.
		 */
		goto retry;
	}

	/*
	 * Get the dictionary pointer for dict1_va2.
	 */
	d2 = follow_dict_pointer(env, dict2_val);

	/*
	 * In case the dict2 is thread-local, and this thread is not the
	 * creator of the dictionary.
	 */
	if (!d2->shared && env != d2->creator) {
		/*
		 * Promote the dictionary to shared.
		 */
		promote_dict_to_shared(env, dict2_val);

		/*
		 * Restart from the beginning.
		 */
		goto retry;
	}

	/*
	 * Start a read. The dictionary pointer will be valid until the next
	 * safepoint, even if a newer structural is published.
	 */
	*dict1 = d1;
	*dict2 = d2;
}

/*
 * ============================================================================
 *  Expand Helpers
 * ============================================================================
 *
 *  Expand allocation invariant:
 *
 *   - If allocation may trigger GC or safepoint synchronization, the storage
 *     pointer obtained before allocation must not be trusted after allocation.
 *   - After allocation, the current storage must be followed again.
 *   - If the operation requires a write lock, the lock must protect the same
 *     current storage that receives the newer publication.
 */

/*
 * Helper: Expand an array.
 */
static INLINE bool
expand_array(
	struct rt_env *env,
	struct rt_value *arr_val,
	struct rt_array *arr,
	size_t size)
{
	struct rt_array *new_arr;
	size_t old_size, i;
	bool locked = false;

	/*
	 * In case the array is thread-local.
	 */
	if (!arr->shared) {
		/*
		 * In case this thread is not the creator of the array.
		 */
		if (env != arr->creator) {
			/*
			 * Promote the array to shared.
			 */
			promote_array_to_shared(env, arr_val);

			/*
			 * Restarting from the beginning is required.
			 */
			return true;
		}

		/*
		 * We don't need a lock because the array is thread-local.
		 */
		goto do_expand;
	}

	/*
	 * The array is shared. Try the array-level lock.
	 */
	if (!add_try_array_write_lock(env, arr)) {
		/*
		 * Failed to lock the array. Restarting from the beginning of
		 * the transaction is required because a race thread may do
		 * expand on the array.
		 */
		return true;
	}
	
	/*
	 * Succesfully locked.
	 */
	locked = true;

	/*
	 * Do expand.
	 */
do_expand:
	/*
	 * Allocate the new array. Multiple GCs may occur in this call(). Each
	 * GC is embraced by om_enter_gc() and om_leave_gc().
	 */
	new_arr = rt_gc_alloc_array(env, size);
	if (new_arr == NULL) {
		/*
		 * Error: out-of-memory.
		 */
		return false;
	}

	/*
	 * Get the array pointer again. The array may be moved by the Copy-GC or
	 * Compaction-GC during the allocation above.
	 */
	arr = follow_array_pointer(env, arr_val);

	/*
	 * Do copy.
	 */
	old_size = arr->size;
	new_arr->size = size;
	if (old_size >= size)
		old_size = size;
	for (i = 0; i < old_size; i++) {
		/*
		 * Copy an entry.
		 */
		new_arr->table[i] = arr->table[i];

		/*
		 * Write barrier for the remember set.
		 */
		rt_gc_array_write_barrier(env, new_arr, i, &new_arr->table[i]);
	}

	/*
	 * If we used a write lock, make the new array locked initially to
	 * prevent it written by another thread after the publish and before the
	 * unlock of the old array.
	 */
	if (locked)
		add_try_array_write_lock(env, new_arr);

	/*
	 * Link the new object, that is, do copy-publish.
	 */
	arr->newer = new_arr;

	/*
	 * Unlock if we used a lock.
	 */
	if (locked) {
		/*
		 * Unlock the old array first, then unlock the new array.
		 */
		sub_array_write_unlock(env, arr);
		sub_array_write_unlock(env, new_arr);
	}

	/*
	 * Restarting from the beginning of the transaction is required.
	 */
	return true;
}

/*
 * Helper: Expand an array.
 */
static INLINE bool
expand_dict(
	struct rt_env *env,
	struct rt_value *dict_val,
	struct rt_dict *dict,
	size_t size)
{
	struct rt_dict *new_dict;
	size_t old_alloc_size;
	size_t i, j, index;
	bool locked = false;

	/*
	 * In case the dictionary is thread-local.
	 */
	if (!dict->shared) {
		/*
		 * In case this thread is not the creator of the dictionary.
		 */
		if (env != dict->creator) {
			/*
			 * Promote the dictionary to shared.
			 */
			promote_dict_to_shared(env, dict_val);

			/*
			 * Restarting from the beginning is required.
			 */
			return true;
		}

		/*
		 * We don't need a lock because the dictionary is thread-local.
		 */
		goto do_expand;
	}

	/*
	 * The dictionary is shared. Try the dictionary-level lock.
	 */
	if (!add_try_dict_write_lock(env, dict)) {
		/*
		 * Failed to lock the dictionary. Restarting from the beginning of
		 * the transaction is required because a race thread may do
		 * expand on the dictionary.
		 */
		return true;
	}
	
	/*
	 * Succesfully locked.
	 */
	locked = true;

	/*
	 * Do expand.
	 */
do_expand:
	/*
	 * Allocate the new dictionary. Multiple GCs may occur in this call(). Each
	 * GC is embraced by om_enter_gc() and om_leave_gc().
	 */
	new_dict = rt_gc_alloc_dict(env, size);
	if (new_dict == NULL) {
		/*
		 * Error: out-of-memory.
		 */
		return false;
	}

	/*
	 * Get the dictionary pointer again. The dictionary may be moved by the
	 * Copy-GC or Compaction-GC during the allocation above.
	 */
	dict = follow_dict_pointer(env, dict_val);

	/*
	 * Do copy/rehash.
	 */
	old_alloc_size = dict->alloc_size;
	for (i = 0; i < old_alloc_size; i++) {
		/*
		 * SKip if an empty slot.
		 */
		if (dict->key[i].type == NOCT_VALUE_INT)
			continue;

		/*
		 * SKip if a removed slot. (tombstone)
		 */
		if (dict->key[i].type == NOCT_VALUE_FLOAT)
			continue;

		/*
		 * Do an in-place write.
		 */
		index = rt_string_hash(dict->key[i].val.str->data) & ((uint32_t)new_dict->alloc_size - 1);
		for (j = index;
		     j != ((index - 1 + new_dict->alloc_size) & (new_dict->alloc_size - 1));
		     j = (j + 1) & ((uint32_t)new_dict->alloc_size - 1)) {
			/*
			 * If an empty entry.
			 */
			if (new_dict->key[j].type == NOCT_VALUE_INT) {
				/*
				 * Copy the key and the value.
				 */
				new_dict->key[j] = dict->key[i];
				new_dict->value[j] = dict->value[i];

				/*
				 * Write barriers for the key and the value.
				 */
				rt_gc_dict_write_barrier(env, new_dict, &new_dict->key[j]);
				rt_gc_dict_write_barrier(env, new_dict, &new_dict->value[j]);

				break;
			}
		}
	}
	new_dict->size = dict->size;

	/*
	 * If we used a write lock, make the new dict locked initially to
	 * prevent it written by another thread after the publish and before the
	 * unlock of the old dictionary.
	 */
	if (locked)
		add_try_dict_write_lock(env, new_dict);

	/*
	 * Link the new object, that is, do copy-publish.
	 */
	dict->newer = new_dict;

	/*
	 * Unlock if we used a lock.
	 */
	if (locked) {
		/*
		 * Unlock the old dictionary first, then unlock the new dictionary.
		 */
		sub_dict_write_unlock(env, dict);
		sub_dict_write_unlock(env, new_dict);
	}

	/*
	 * Restarting from the beginning of the transaction is required.
	 */
	return true;
}

/*
 * ===========================================================================
 *  Array
 * ===========================================================================
 */

/*
 * Make an empty array.
 */
bool
om_make_array(
	struct rt_env *env,
	struct rt_value *val)
{
	struct rt_array *arr;
	const uint32_t START_SIZE = 16;

	/*
	 * Allocate an array. This may occur GC.
	 */
	arr = rt_gc_alloc_array(env, START_SIZE);
	if (arr == NULL) {
		rt_out_of_memory(env);
		return false;
	}

	/*
	 * Here, this thread is "in-flight" and GC won't be executed
	 * in other threads.
	 */

	/*
	 * Setup a value.
	 */
	val->type = NOCT_VALUE_ARRAY;
	val->val.arr = arr;

	return true;
}

/*
 * Get the size of an array.
 */
bool
om_get_array_size(
	struct rt_env *env,
	struct rt_value *arr_val,
	size_t *size)
{
	struct rt_array *arr;

	/*
	 * Start an array read.
	 */
	arr = start_array_read(env, arr_val);

	*size = arr->size;

	return true;
}

/*
 * Read an array element.
 */
bool
om_read_array(
	struct rt_env *env,
	struct rt_value *arr_val,
	size_t index,
	struct rt_value *val)
{
	struct rt_array *arr;
	int seq1, seq2;

	/*
	 * Start an array read.
	 */
	arr = start_array_read(env, arr_val);

	/*
	 * We can do read access starting from here. The array will not be
         * collected by GC until the next safepoint.
	 */
	if (index >= arr->alloc_size) {
		/*
		 * Index is out-of-range.
		 */
		return false;
	}

	/*
	 * Check whether the array is shared or not.
	 */
	if (!arr->shared) {
		/*
		 * Thread-local, not shared.
		 * Read without SeqLock.
		 */
		*val = arr->table[index];
	} else {
		/*
		 * Shared.
		 * Read with SeqLock.
		 */
		do {
			seq1 = get_array_seqlock(env, arr);
			if (seq1 & 1)
				continue;

			*val = arr->table[index];

			seq2 = get_array_seqlock(env, arr);

		} while ((seq1 != seq2)   ||    /* Race write has occurred, or */
			 (seq2 & 1) != 0);      /* Read an unstable value. */
	}

	/*
	 * Succeeded.
	 */
	return true;
}

/*
 * Write an array element.
 */
bool
om_write_array(
	struct rt_env *env,
	struct rt_value *arr_val,
	size_t index,
	struct rt_value *val)
{
	struct rt_array *arr;
	size_t new_size;

retry:
	/*
	 * Start an array read.
	 */
	arr = start_array_read(env, arr_val);

	/*
	 * In case expand is required. Note that in-palce write and expand are
	 * theoretically separate operations in this object model. However, we
	 * integrate them into a single om_write_array() function and do them
	 * sequentially to reduce and optimize the safepoint handling.
	 */
	if (index >= arr->alloc_size) {
		/*
		 * Array expansion is required. Determine the new size.
		 */
		new_size = arr->alloc_size * 2;
		while (index > new_size * 2)
			new_size *= 2;

		/*
		 * Try expanding.
		 */
		if (!expand_array(env, arr_val, arr, new_size)) {
			/*
			 * Out-of-memory error.
			 */
			return false;
		}

		/*
		 * Succeeded to expand. Restart from the beginning.
		 */
		goto retry;
	}

	/*
	 * In case the array is thread-local.
	 */
	if (!arr->shared) {
		/*
		 * In case this thread is not the creator of the array.
		 */
		if (env != arr->creator) {
			/*
			 * Promote the array to shared.
			 */
			promote_array_to_shared(env, arr_val);

			/*
			 * Restart from the beginning.
			 */
			goto retry;
		}

		/*
		 * We don't need a lock because the array is thread-local.
		 * Do a write now without SeqLock.
		 */
		arr->table[index] = *val;

		/*
		 * Write barrier for the remember set.
		 *
		 * Write barrier invariant:
		 *
		 *  - Any pointer value copied into newly allocated storage must
		 *    be recorded before the storage is published to other
		 *    threads.
		 *  - For copy-publish expansion, all write barriers for copied
		 *    entries must be completed before publishing newer.
		 */
		rt_gc_array_write_barrier(env, arr, index, val);

		/*
		 * Succeeded.
		 */
		return true;
	}

	/*
	 * The array is shared. Try the array-level lock.
	 */
	if (!add_try_array_write_lock(env, arr)) {
		/*
		 * Failed to lock the array. Go back to the beginning.
		 */
		goto retry;
	}

	/*
	 * Succesfully locked.
	 * Do a write now with SeqLock.
	 */
	assert(arr->seqlock % 2 == 0);
	increment_array_seqlock(env, arr);	/* Make unstable. (LSB set) */
	arr->table[index] = *val;		/* Store two machine words. */
	increment_array_seqlock(env, arr);	/* Make stable. (LSB cleared) */

	/*
	 * Write barrier for the remember set.
	 */
	rt_gc_array_write_barrier(env, arr, index, val);

	/*
	 * Unlock the array.
	 */
	sub_array_write_unlock(env, arr);

	/*
	 * Succeeded.
	 */
	return true;
}

/*
 * Resize an array.
 */
bool
om_resize_array(
	struct rt_env *env,
	struct rt_value *arr_val,
	size_t req_size)
{
	struct rt_array *arr;

	/*
	 * Start an array read.
	 */
	arr = start_array_read(env, arr_val);

	/*
	 * Try expanding. (including size reduction)
	 */
	if (!expand_array(env, arr_val, arr, req_size)) {
		/*
		 * Out-of-memory error.
		 */
		return false;
	}

	/*
	 * Succeeded to expand.
	 */
	return true;
}

/*
 * Make a shallow copy of an array.
 */
bool
om_copy_array(
	struct rt_env *env,
	struct rt_value *dst,
	struct rt_value *src)
{
	struct rt_array *d, *s;
	size_t size, i;

	/*
	 * Start a source array read.
	 */
	s = start_array_read(env, src);

	/*
	 * Get the source array allocation size.
	 * Finish the read.
	 */
	size = s->alloc_size;

	/*
	 * Make a dictionary.
	 */
	d = rt_gc_alloc_array(env, size);
	if (d == NULL) {
		/*
		 * Out-of-memory error.
		 */
		return false;
	}

	/*
	 * "s" may be invalid after a GC in rt_gc_alloc_array() and we need to
	 * call start_array_read() again to obtain a valid pointer.
	 *
	 * Before that, we need to pin "d" to the destination rt_value. This is
	 * required to avoid d to be collected by GC in the call of
	 * start_dict_read().
	 */
	dst->type = NOCT_VALUE_ARRAY;
	dst->val.arr = d;

	/*
	 * Start a source dictionary read again.
	 */
	s = start_array_read(env, src);

	/*
	 * Copy the dictionary with write-barrier.
	 *
	 * Invariant:
	 *
	 *  - dst usually points to a thread-local stack entry.
	 *  - Even if d points to a global variable in a native code, the
	 *    access is mutually excluded.
	 *  - Thus, storing d to dst is not a immediate publish to other
	 *    threads.
	 *  - In conclusion, we don't need a write lock here.
	 */
	for (i = 0; i < size; i++) {
		/*
		 * Copy an item.
		 */
		d->table[i] = s->table[i];
		d->size++;

		/*
		 * Write barrier.
		 */
		rt_gc_array_write_barrier(env, d, i, &d->table[i]);
	}

	return true;
}

/*
 * ===========================================================================
 *  Dictionary
 * ===========================================================================
 *
 * Dictionary slot state invariant:
 *
 *  - EMPTY slots terminate lookup.
 *  - TOMBSTONE slots do not terminate lookup.
 *  - OCCUPIED slots have a valid string key and value.
 *  - EMPTY and TOMBSTONE are represented by reserved rt_value
 *    tags and must never be used as real dictionary keys.
 *
 * Dictionary entry publication invariant:
 *
 *  - An empty key slot means the entry is not visible to readers.
 *  - For insertion, the value must be initialized before the key becomes
 *    visible.
 *  - The key is the publication point of a dictionary entry.
 *  - Readers must not observe a visible key with an uninitialized value.
 *
 * Dictionary key publication invariant:
 *
 *  - key.type is the publication word of a dictionary entry.
 *  - EMPTY/TOMBSTONE key.type means the entry is invisible to readers.
 *  - Writers initialize key.val.rts before publishing key.type.
 *  - Writers complete required write barriers before publishing key.type.
 *  - Readers acquire-load key.type first.
 *  - Readers may read key.val.rts only after observing key.type == STRING.
 */

/*
 * Make an empty array.
 */
bool
om_make_dict(
	struct rt_env *env,
	struct rt_value *val)
{
	struct rt_dict *dict;
	const uint32_t START_SIZE = 16;

	/*
	 * Allocate a dictionary. This may occur GC.
	 */
	dict = rt_gc_alloc_dict(env, START_SIZE);
	if (dict == NULL) {
		rt_out_of_memory(env);
		return false;
	}

	/*
	 * Here, this thread is "in-flight" and GC won't be executed
	 * in other threads.
	 */

	/*
	 * Setup a value.
	 */
	val->type = NOCT_VALUE_DICT;
	val->val.dict = dict;

	return true;

}

/*
 * Make an empty array.
 */
bool
om_get_dict_alloc_size(
	struct rt_env *env,
	struct rt_value *val,
	size_t *size)
{
	struct rt_dict *dict;

	/*
	 * Start a dictionary read.
	 */
	dict = start_dict_read(env, val);

	/*
	 * Get the allocated size.
	 */
	*size = dict->alloc_size;

	return true;
}

/*
 * Check if a key exists in a dictionary.
 */
bool
om_check_dict_key(
	struct rt_env *env,
	struct rt_value *dict_val,
	struct rt_value *key,
	bool *ret)
{
	struct rt_dict *dict;
	size_t size, index, i;

	/*
	 * Start a dictionary read.
	 */
	dict = start_dict_read(env, dict_val);

	size = dict->alloc_size;

	/*
	 * Search for the key.
	 */
	index = key->val.str->hash & (uint32_t)(dict->alloc_size - 1);
	for (i = index;
	     i != ((index - 1 + size) & (size - 1));
	     i = (i + 1) & ((uint32_t)size - 1)) {
		int type;
		struct rt_string *rts;

		/*
		 * Get the key type. We avoid reading the entire value of the
		 * key due to tear. A key consists of two machine words. The
		 * first word is the type, and the second word is the rt_string
		 * pointer. Readers must read the type first. Writers must write
		 * the string pointer first when inserting a key. Writing a type
		 * for a key means a publish for insertion.
		 */
		if (!dict->shared)
			type = dict->key[i].type;
		else
			type = atomic_load_acquire_int(&dict->key[i].type);

		/*
		 * Skip if a removed slot. (tombstone)
		 */
		if (type == NOCT_VALUE_FLOAT)
			continue;

		/*
		 * Stop if an empty slot.
		 */
		if (type == NOCT_VALUE_INT) {
			/*
			 * Failed: Key not found.
			 */
			*ret = false;
			break;
		}

		/*
		 * Get the key string pointer at the second word.
		 */
		if (!dict->shared)
			rts = dict->key[i].val.str;
		else
			rts = atomic_load_acquire_ptr((void **)&dict->key[i].val.str);

		if (rts->len == key->val.str->len &&
		    rts->hash == key->val.str->hash &&
		    strcmp(rts->data, key->val.str->data) == 0) {
			/*
			 * Found the key.
			 */
			*ret = true;
			break;
		}
	}

	return ret;
}

/*
 * Read a dictionary element.
 */
bool
om_read_dict(
	struct rt_env *env,
	struct rt_value *dict_val,
	struct rt_value *key,
	struct rt_value *val)
{
	if (!om_read_dict_with_hash(env,
				    dict_val,
				    key->val.str->data,
				    key->val.str->len,
				    key->val.str->hash,
				    val))
		return false;

	return true;
}

/*
 * Read a dictionary element.
 */
bool
om_read_dict_with_hash(
	struct rt_env *env,
	struct rt_value *dict_val,
	const char *key,
	size_t key_len,
	uint32_t key_hash,
	struct rt_value *val)
{
	struct rt_dict *dict;
	size_t index, i;
	int seq1, seq2;

	/*
	 * Start a dictionary read.
	 */
	dict = start_dict_read(env, dict_val);

	/*
	 * Search for the key.
	 */
	index = key_hash & (uint32_t)(dict->alloc_size - 1);
	for (i = index;
	     i != ((index - 1 + dict->alloc_size) & (dict->alloc_size - 1));
	     i = (i + 1) & ((uint32_t)dict->alloc_size - 1)) {
		int type;
		struct rt_string *rts;

		/*
		 * Get the key type. We avoid reading the entire value of the
		 * key due to tear. A key consists of two machine words. The
		 * first word is the type, and the second word is the rt_string
		 * pointer. Readers must read the type first. Writers must write
		 * the string pointer first when inserting a key. Writing a type
		 * for a key means a publish for insertion.
		 */
		if (!dict->shared)
			type = dict->key[i].type;
		else
			type = atomic_load_acquire_int(&dict->key[i].type);

		/*
		 * Dictionary slot state invariant:
		 *
		 *  - EMPTY slots terminate lookup.
		 *  - TOMBSTONE slots do not terminate lookup.
		 *  - OCCUPIED slots have a valid string key and value.
		 *  - EMPTY and TOMBSTONE are represented by reserved rt_value
		 *    tags and must never be used as real dictionary keys.
		 */

		/*
		 * Skip if a removed slot. (tombstone)
		 */
		if (type == NOCT_VALUE_FLOAT)
			continue;

		/*
		 * Stop if an empty slot.
		 */
		if (type == NOCT_VALUE_INT) {
			/*
			 * Failed: Key not found.
			 */
			return false;
		}

		/*
		 * Get the key string pointer at the second word.
		 */
		if (!dict->shared)
			rts = dict->key[i].val.str;
		else
			rts = atomic_load_acquire_ptr((void **)&dict->key[i].val.str);

		if (rts->len == key_len &&
		    rts->hash == key_hash &&
		    strcmp(rts->data, key) == 0) {
			/*
			 * Found the key.
			 */

			/*
			 * Check whether the dictionary is shared or not.
			 */
			if (!dict->shared) {
				/*
				 * Thread-local, not shared.
				 * Read without SeqLock.
				 */
				*val = dict->value[i];
			} else {
				/*
				 * Shared.
				 * Read with SeqLock.
				 */
				do {
					seq1 = get_dict_seqlock(env, dict);
					if (seq1 & 1)
						continue;

					*val = dict->value[i];

					seq2 = get_dict_seqlock(env, dict);

				} while ((seq1 != seq2)   ||    /* Race write has occurred, or */
					 (seq2 & 1) != 0);      /* Read an unstable value. */
			}
			break;
		}
	}

	/*
	 * Succeeded.
	 */
	return true;
}

/*
 * Read a dictionary element by a slot index. (for traverse)
 */
bool
om_read_dict_index(
	struct rt_env *env,
	struct rt_value *dict_val,
	size_t index,
	struct rt_value *key,
	struct rt_value *val)
{
	struct rt_dict *dict;
	int seq1, seq2;

	/*
	 * Start a dictionary read.
	 */
	dict = start_dict_read(env, dict_val);

	/*
	 * Check for the size.
	 */
	if (index >= dict->alloc_size) {
		/*
		 * Index is out-of-bound.
		 */
		return false;
	}

	/*
	 * Check whether the dictionary is shared or not.
	 */
	if (!dict->shared) {
		/*
		 * Thread-local, not shared.
		 * Read without SeqLock.
		 */
		*val = dict->value[index];
		*key = dict->key[index];
	} else {
		/*
		 * Shared.
		 */

		int type = atomic_load_acquire_int(&dict->key[index].type);

		/*
		 * Skip if a removed or emtpty slot.
		 */
		if (type != NOCT_VALUE_STRING) {
			/*
			 * No entry at the index.
			 */
			return false;
		}

		/*
		 * Get the key string pointer at the second word.
		 */
		do {
			seq1 = get_dict_seqlock(env, dict);
			if (seq1 & 1)
				continue;

			*val = dict->value[index];

			seq2 = get_dict_seqlock(env, dict);

		} while ((seq1 != seq2)   ||    /* Race write has occurred, or */
			 (seq2 & 1) != 0);      /* Read an unstable value. */
	}

	/*
	 * Successfully read the key and the value at the index.
	 */
	return true;
}

/*
 * Write a dictionary element.
 */
bool
om_write_dict(
	struct rt_env *env,
	struct rt_value *dict_val,
	struct rt_value *key,
	struct rt_value *val)
{
	struct rt_dict *dict;
	size_t new_size;
	size_t index, i;
	bool is_insertion;
	bool found_tombstone;
	bool locked;
	size_t first_tombstone_index;

retry:
	/*
	 * Start a dictionary read.
	 */
	dict = start_dict_read(env, dict_val);

	/*
	 * In case the dictionary is thread-local.
	 */
	if (!dict->shared) {
		/*
		 * In case this thread is not the creator of the dictionary.
		 */
		if (env != dict->creator) {
			/*
			 * Promote the dictionary to shared.
			 */
			promote_dict_to_shared(env, dict_val);

			/*
			 * Restart from the beginning.
			 */
			goto retry;
		}

		/*
		 * We don't need a lock because the dictionary is thread-local.
		 */
		locked = false;

		/*
		 * Proceed to the in-place write phase.
		 */
		goto in_place_write_phase;
	}

	/*
	 * The dictionary is shared. Try the dictionary-level lock.
	 */
	if (!add_try_dict_write_lock(env, dict)) {
		/*
		 * Failed to lock the dictionary. Go back to the beginning.
		 */
		goto retry;
	}
	locked = true;

	/*
	 * In-place write phase.
	 *
	 * Dictionary entry publication invariant:
	 *
	 *  - An empty key slot means the entry is not visible to readers.
	 *  - For insertion, the value must be initialized before the key becomes
	 *    visible.
	 *  - The key is the publication point of a dictionary entry.
	 *  - Readers must not observe a visible key with an uninitialized value.
	 *
	 * Updating an existing key may update only the value under SeqLock.
	 * Inserting a new key must publish the key only after the value and write
	 * barriers are complete.
	 */
in_place_write_phase:
	/*
	 * If 75% of the space is used, go to the expand phase.
	 */
	if (dict->size >= dict->alloc_size / 4 * 3)
		goto expand_phase;

	/*
	 * Make sure we have at least one space.
	 */
	if (dict->size == dict->alloc_size) {
		/*
		 * No space. This is strange because we expand a dictionary at
		 * the 75% usage of the allocation. Make an assertion error for
		 * a debug build, and expand the dictionary for a release build.
		 */
		assert(NO_SPACE_FOR_DICTIONARY);
		goto expand_phase;
	}

	/*
	 * Search an index for in-place write.
	 */
	index = key->val.str->hash & (uint32_t)(dict->alloc_size - 1);
	found_tombstone = false;
	for (i = index;
	     i != ((index - 1 + dict->alloc_size) & (dict->alloc_size - 1));
	     i = (i + 1) & ((uint32_t)dict->alloc_size - 1)) {
		/*
		 * Found a tombstone for a removed slot, skip for now with a
		 * mark.
		 */
		if (dict->key[i].type == NOCT_VALUE_FLOAT) {
			if (!found_tombstone) {
				found_tombstone = true;
				first_tombstone_index = i;
			}
			continue;
		}

		/*
		 * Found an empty slot. Proceed to the expand phase if 75% of
		 * the space is used.
		 */
		if (dict->key[i].type == NOCT_VALUE_INT) {
			/*
			 *  Proceed to write. (insertion)
			 */
			is_insertion = true;
			break;
		}

		/*
		 * Found the same key. Proceed to write.
		 */
		if (dict->key[i].val.str->len == key->val.str->len &&
		    dict->key[i].val.str->hash == key->val.str->hash &&
		    strcmp(dict->key[i].val.str->data, key->val.str->data) == 0) {
			/*
			 * Proceed to write. (non-insertion)
			 */
			is_insertion = false;
			break;
		}
	}
	if (found_tombstone) {
		index = first_tombstone_index;
		is_insertion = true;
	}

	/*
	 * Execute an in-place write. Check whether the dictionary is shared or
	 * not.
	 */
	if (!dict->shared) {
		/*
		 * Thread-local, not shared. Write without SeqLock.
		 */
		dict->value[i] = *val;
		if (is_insertion)
			dict->key[i] = *key;

		/*
		 * Write barrier for the remember set.
		 */
		rt_gc_dict_write_barrier(env, dict, key);
		rt_gc_dict_write_barrier(env, dict, val);
	} else {
		/*
		 * Shared.
		 */

		/*
		 * For the value, write with SeqLock to protect from tear.
		 */
		increment_dict_seqlock(env, dict);
		dict->value[i] = *val;
		increment_dict_seqlock(env, dict);

		/*
		 * Value write barrier for the remember set.
		 */
		rt_gc_dict_write_barrier(env, dict, val);

		/*
		 * For the key, write the key's string pointer first, then write
		 * the key's type last.
		 */
		if (is_insertion) {
			/*
			 * Key write barrier for the remember set.
			 */
			rt_gc_dict_write_barrier(env, dict, key);

			/*
			 * Write two words. The second word first.
			 */
			atomic_store_release_ptr((void **)&dict->key[i].val.str, key->val.str);
			atomic_store_release_int(&dict->key[i].type, NOCT_VALUE_STRING);
		}

		/*
		 * Unlock.
		 */
		sub_dict_write_unlock(env, dict);
	}

	/*
	 * Write barrier for the remember set.
	 */
	rt_gc_dict_write_barrier(env, dict, key);
	rt_gc_dict_write_barrier(env, dict, val);

	/*
	 * Succeeded, return here. Note that this is the only successful
	 * exit of this function.
	 */
	return true;

	/*
	 * Expand phase.
	 */
expand_phase:
	/*
	 * Determine the new size.
	 */
	new_size = dict->alloc_size * 2;

	/*
	 * Try expanding.
	 */
	if (!expand_dict(env, dict_val, dict, new_size)) {
		/*
		 * Out-of-memory error.
		 */
		return false;
	}

	/*
	 * Unlock.
	 */
	if (locked) {
		sub_dict_write_unlock(env, dict);
		locked = false;
	}

	/*
	 * Succeeded to expand. Restart from the beginning.
	 */
	goto retry;
}

/*
 * Write a dictionary element.
 */
bool
om_write_dict_with_hash(
	struct rt_env *env,
	struct rt_value *dict_val,
	const char *key,
	size_t len,
	uint32_t hash,
	struct rt_value *val)
{
	struct rt_value s;

	rt_pin_local(env, &s);

	if (!rt_make_string_with_hash(env, &s, key, len, hash)) {
		rt_unpin_local(env, &s);
		return false;
	}

	if (!om_write_dict(env, dict_val, &s, val)) {
		rt_unpin_local(env, &s);
		return false;
	}

	rt_unpin_local(env, &s);

	return true;
}

/*
 * Write a dictionary element.
 */
bool
om_erase_dict_entry(
	struct rt_env *env,
	struct rt_value *dict_val,
	struct rt_value *key)
{
	struct rt_dict *dict;
	size_t index, i;
	bool is_not_found;
	bool locked;

retry:
	/*
	 * Start a dictionary read.
	 */
	dict = start_dict_read(env, dict_val);

	/*
	 * In case the dictionary is thread-local.
	 */
	if (!dict->shared) {
		/*
		 * In case this thread is not the creator of the dictionary.
		 */
		if (env != dict->creator) {
			/*
			 * Promote the dictionary to shared.
			 */
			promote_dict_to_shared(env, dict_val);

			/*
			 * Restart from the beginning.
			 */
			goto retry;
		}

		/*
		 * We don't need a lock because the dictionary is thread-local.
		 */
		locked = false;

		/*
		 * Proceed to the in-place write phase.
		 */
		goto in_place_write_phase;
	}

	/*
	 * The dictionary is shared. Try the dictionary-level lock.
	 */
	if (!add_try_dict_write_lock(env, dict)) {
		/*
		 * Failed to lock the dictionary. Go back to the beginning.
		 */
		goto retry;
	}
	locked = true;

	/*
	 * We first try the in-place write phase, then move to the expand phase
	 * if required. After that, we move to the in-place write phase. This is
	 * looped.
	 */

	/*
	 * In-place write phase.
	 */
in_place_write_phase:
	/*
	 * Search an index for in-place write.
	 */
	index = key->val.str->hash & (uint32_t)(dict->alloc_size - 1);
	for (i = index;
	     i != ((index - 1 + dict->alloc_size) & (dict->alloc_size - 1));
	     i = (i + 1) & ((uint32_t)dict->alloc_size - 1)) {
		/*
		 * Found a tombstone for a removed slot, skip for now with a
		 * mark.
		 */
		if (dict->key[i].type == NOCT_VALUE_FLOAT)
			continue;

		/*
		 * Found the same key. Proceed to write.
		 */
		if (dict->key[i].val.str->len == key->val.str->len &&
		    dict->key[i].val.str->hash == key->val.str->hash &&
		    strcmp(dict->key[i].val.str->data, key->val.str->data) == 0) {
			/*
			 * Proceed to write. (non-insertion)
			 */
			is_not_found = false;
			break;
		}
	}
	if (is_not_found) {
		/*
		 * Unlock.
		 */
		if (locked)
			sub_dict_write_unlock(env, dict);

		/*
		 * Failed: Key not found.
		 */
		return false;
	}

	/*
	 * Execute an in-place write. Check whether the dictionary is shared or
	 * not.
	 */
	if (!dict->shared) {
		/*
		 * Thread-local, not shared. Write a tombstone.
		 */
		dict->key[i].type = NOCT_VALUE_FLOAT;
		dict->key[i].val.f = 0.0f;
		dict->value[i].type = NOCT_VALUE_INT;
		dict->value[i].val.i = 0;
	} else {
		/*
		 * Shared.
		 */
		struct rt_value zero;
		memset(&zero, 0, sizeof(zero));

		/*
		 * For the value, write with SeqLock to protect from tear.
		 */
		increment_dict_seqlock(env, dict);
		dict->value[i] = zero;
		increment_dict_seqlock(env, dict);

		/*
		 * For the key, write the key's type only. Float is a tombstone
		 * marker.
		 */
		atomic_store_release_int(&dict->key[i].type, NOCT_VALUE_FLOAT);

		/*
		 * Unlock.
		 */
		sub_dict_write_unlock(env, dict);
	}

	/*
	 * Succeeded.
	 */
	return true;
}

/*
 * Make a shallow copy of a dictionary.
 */
bool
om_copy_dict(
	struct rt_env *env,
	struct rt_value *dst,
	struct rt_value *src)
{
	struct rt_dict *d, *s;
	size_t size, i;

	/*
	 * Start a source dictionary read.
	 */
	s = start_dict_read(env, src);

	/*
	 * Get the source dictionary allocation size.
	 * Finish the read.
	 */
	size = s->alloc_size;

	/*
	 * Make a dictionary.
	 */
	d = rt_gc_alloc_dict(env, size);
	if (d == NULL) {
		/*
		 * Out-of-memory error.
		 */
		return false;
	}

	/*
	 * "s" may be invalid after a GC in rt_gc_alloc_dict() and we need to
	 * call start_dict_read() again to obtain a valid pointer.
	 *
	 * Before that, we need to pin d to the destination rt_value. This is
	 * required to avoid d to be collected by GC in the call of
	 * start_dict_read().
	 */
	dst->type = NOCT_VALUE_DICT;
	dst->val.dict = d;

	/*
	 * Start a source dictionary read again.
	 */
	s = start_dict_read(env, src);

	/*
	 * Copy the dictionary with write-barrier.
	 *
	 * Invariant:
	 *
	 *  - dst usually points to a thread-local stack entry.
	 *  - Even if d points to a global variable in a native code, the
	 *    access is mutually excluded.
	 *  - Thus, storing d to dst is not a immediate publish to other
	 *    threads.
	 *  - In conclusion, we don't need a write lock here.
	 */
	for (i = 0; i < size; i++) {
		/*
		 * Skip an empty or removed entry.
		 */
		if (s->key[i].type != NOCT_VALUE_STRING)
			continue;

		/*
		 * Copy an item.
		 */
		d->key[i] = s->key[i];
		d->value[i] = s->value[i];
		d->size++;

		/*
		 * Write barrier.
		 */
		rt_gc_dict_write_barrier(env, d, &d->key[i]);
		rt_gc_dict_write_barrier(env, d, &d->value[i]);
	}

	return true;
}

/*
 * Merges a dictionary.
 */
bool
om_merge_dict(
	struct rt_env *env,
	struct rt_value *dst,
	struct rt_value *src1,
	struct rt_value *src2)
{
	struct rt_dict *d, *s1, *s2;
	size_t src1_size, src2_size, dst_size;
	size_t i, j, k, index;

	/*
	 * Start a source dictionary read.
	 */
	start_dict_read2(env, src1, src2, &s1, &s2);

	/*
	 * Get the source dictionary item sizes. (not allocation sizes)
	 * Finish the read.
	 */
	src1_size = s1->size;
	src2_size = s2->size;

	/*
	 * Determine the destination size;
	 */
	dst_size = 1;
	while (dst_size < src1_size || dst_size < src2_size)
		dst_size *= 2;

	/*
	 * Make a dictionary.
	 */
	d = rt_gc_alloc_dict(env, dst_size);
	if (d == NULL) {
		/*
		 * Out-of-memory error.
		 */
		return false;
	}

	/*
	 * s1 and s2 may be invalid after a GC in rt_gc_alloc_dict() and we need
	 * to call start_dict_read() again to obtain a valid pointer.
	 *
	 * Before that, we need to pin d to the destination rt_value. This is
	 * required to avoid d to be collected by GC in the call of
	 * start_dict_read().
	 */
	dst->type = NOCT_VALUE_DICT;
	dst->val.dict = d;

	/*
	 * Start a source dictionary read again.
	 */
	start_dict_read2(env, src1, src2, &s1, &s2);

	/*
	 * Copy the dictionary with write-barrier.
	 *
	 * Invariant:
	 *
	 *  - dst usually points to a thread-local stack entry.
	 *  - Even if d points to a global variable in a native code, the
	 *    access is mutually excluded.
	 *  - Thus, storing d to dst is not a immediate publish to other
	 *    threads.
	 *  - In conclusion, we don't need a write lock here.
	 */
	for (i = 0; i < 2; i++) {
		struct rt_dict *sn;
		if (i == 0)
			sn = s1;
		else
			sn = s2;
			
		for (j = 0; j < s1->alloc_size; j++) {
			/*
			 * Skip an empty or removed entry.
			 */
			if (sn->key[j].type != NOCT_VALUE_STRING)
				continue;

			/*
			 * In-place write.
			 */
			index = sn->key[j].val.str->hash & (uint32_t)(d->alloc_size - 1);
			for (k = index;
			     k != ((index - 1 + d->alloc_size) & (d->alloc_size - 1));
			     k = (j + 1) & ((uint32_t)d->alloc_size - 1)) {
				/*
				 * Skip an empty slot
				 */
				if (d->key[i].type != NOCT_VALUE_STRING)
					break;

				/*
				 * Found the same key.
				 */
				if (d->key[i].val.str->len == sn->key[j].val.str->len &&
				    d->key[i].val.str->hash == sn->key[j].val.str->hash &&
				    strcmp(d->key[i].val.str->data, sn->key[j].val.str->data) == 0)
					break;
			}

			/*
			 * Copy an item.
			 */
			d->key[k] = sn->key[j];
			d->value[k] = sn->value[j];
			d->size++;

			/*
			 * Write barrier.
			 */
			rt_gc_dict_write_barrier(env, d, &d->key[k]);
			rt_gc_dict_write_barrier(env, d, &d->value[k]);
		}
	}

	return true;
}

/*
 * ===========================================================================
 *  GC
 * ===========================================================================
 */

/*
 * Enter a GC execution.
 */
bool
om_enter_gc(
	struct rt_env *env,
	int gc_level)
{
	/*
	 * Check whether this is a nested GC, or not.
	 *
	 * Note that nesting is required in case:
	 *  - We are doing a young copying GC, and,
	 *  - We need to promote a young object to the old region, and,
	 *  - We don't have enough space in the old region, then,
	 *  - We need to run an old GC inside the current young GC.
	 */
	if (!check_if_nested_gc(env)) {
		/*
		 * Not nested. Enter a safepoint.
		 */
		enter_safepoint(env);

		/*
		 * Promote to the STW-executor.
		 */
		promote_to_stw_executor(env);

		/*
		 * Wait for all threads to enter safepoints.
		 */
		ensure_all_threads_reach_safepoints(env);
	} else {
		/*
		 * Nested. This thread is already the STW-executor.
		 */

		/*
		 * Check for the GC level. Lower level GC must be skipped.
		 */
		if (gc_level <= env->vm->gc_level) {
			/*
			 * False means no need for GC execution.
			 */
			return false;
		}
	}

	/*
	 * Count-up for nesting.
	 */
	increment_gc_in_progress_count(env);

	/*
	 * True means need for GC execution.
	 */
	return true;
}

/*
 * Leave a GC execution.
 */
void
om_leave_gc(
	struct rt_env *env)
{
	/*
	 * Count-down for nesting.
	 */
	decrement_gc_in_progress_count(env);

	/*
	 * Check if this call is the toplevel of the nesting.
	 */
	if (!check_if_nested_gc(env)) {
		/*
		 * Demote from the STW-executor.
		 */
		demote_from_stw_executor(env);

		/*
		 * Leave the safepoint.
		 */
		leave_safepoint(env);

		/*
		 * Do a safepoint synchronization.
		 */
		synchronize_safepoints(env);
	}
}

/*
 * ===========================================================================
 *  Safepoint
 * ===========================================================================
 */

/*
 * Make a safepoint.
 */
void
om_safepoint(
	struct rt_env *env)
{
	synchronize_safepoints(env);
}

/*
 * ===========================================================================
 *  Other
 * ===========================================================================
 */

/*
 * Make thread in-flight for thread creation.
 */
void
om_init_env(
	struct rt_env *env)
{
	leave_safepoint(env);
	synchronize_safepoints(env);
}
/*
 * ============================================================================
 *  Memory Ordering Memo
 * ============================================================================
 *
 *  Atomic operations are safely wrapped in our local helper functions, so you
 *  should generally use them. However, here is a quick conceptual primer on the
 *  memory ordering used in this runtime.
 *
 *  In short:
 *   - The side sending a flag/publishing data uses "release".
 *   - The side receiving a flag/observing data uses "acquire".
 *
 *  Release Ordering (Store-Release):
 *
 *   - Guarantees that all preceding reads and writes are serialized and
 *     visible BEFORE this store operation. They cannot be reordered after it.
 *
 *   - x86 (TSO): Compiles to a standard move/store instruction. No extra
 *     hardware fence is required because x86 inherently disallows Store-Store
 *     and Load-Store reordering.
 *
 *   - ARM / RISC-V: Emits a write fence (e.g., 'dmb ishst') before the store,
 *     or utilizes a specialized store-release instruction (e.g., 'stlr').
 *
 *  Acquire Ordering (Load-Acquire):
 *
 *   - Guarantees that all subsequent reads and writes are serialized and
 *     executed AFTER this load operation. They cannot be reordered before it.
 *
 *   - x86 (TSO): Compiles to a standard move/load instruction. No extra
 *     hardware fence is required because x86 inherently disallows Load-Load
 *     and Store-Load reordering.
 *
 *   - ARM / RISC-V: Emits a read fence (e.g., 'dmb ish') after the load,
 *     or utilizes a specialized load-acquire instruction (e.g., 'ldar').
 *
 * ============================================================================
 */
