# Ray Tracer

A physically-based ray tracer written in C++17, featuring a full BVH acceleration structure with SAH-based splitting, a flattened linear tree for cache-efficient traversal, and a thread pool that parallelises rendering over image tiles.

---

## Table of Contents

- [Brief Overview](#brief-overview)
- [BVH Acceleration Structure](#bvh-acceleration-structure)
  - [Phase 1 — Build: Recursive Tree Construction](#phase-1--build-recursive-tree-construction)
  - [Surface Area Heuristic (SAH)](#surface-area-heuristic-sah)
  - [Memory Arena](#memory-arena)
  - [Phase 2 — Flatten: Tree to Linear Array](#phase-2--flatten-tree-to-linear-array)
  - [Phase 3 — Traverse: Iterative Ray Intersection](#phase-3--traverse-iterative-ray-intersection)
  - [Ray–AABB Intersection](#rayaabb-intersection)
- [Thread Pool](#thread-pool)
  - [Design](#design)
  - [Tile-Based Rendering](#tile-based-rendering)
  - [Lifecycle](#lifecycle)
- [Benchmarks](#benchmarks)
- [Project Structure](#project-structure)

---

## Brief Overview

The tracer renders a scene of spheres with three material types — Lambertian diffuse, metal, and dielectric (glass) — using recursive path tracing with configurable samples per pixel and bounce depth. The camera supports configurable field of view, look-from/look-at positioning, and anti-aliasing via jittered sampling. Materials implement physically-based scattering: Lambertians scatter into the hemisphere, metals reflect with a fuzz parameter, and dielectrics refract using Snell's law with Schlick approximation for reflectance. Output is written as PPM to stdout.

---

## BVH Acceleration Structure

A Bounding Volume Hierarchy (BVH) wraps every primitive in an axis-aligned bounding box (AABB) and organises them into a binary tree, so a ray only needs to test the small subset of primitives whose bounding boxes it actually intersects. Without a BVH, ray–scene intersection is O(n) per ray. With one it becomes O(log n).

The implementation is split into three cleanly separated phases: **build**, **flatten**, and **traverse**.

---

### Phase 1 — Build: Recursive Tree Construction

**Files:** `src/bvh_accelerator.cpp`, `src/bvhrecursivebuild.cpp`

Construction begins in `BVHAccelerator::BVHAccelerator`. Each primitive first gets wrapped in a `BVHhittableInfo` struct that precomputes its bounding box and centroid:

```cpp
struct BVHhittableInfo {
    size_t hittableno;
    Bounds3f bounds;
    point3 centroid;  // = 0.5 * bmin + 0.5 * bmax
};
```

`recursiveBuild` is then called on the full range `[0, n)` of that array and builds a tree of `BVHbuildnode`s allocated from a `MemoryArena` (see below). Each recursive call:

**1. Computes the union of all bounds in the range.**

**2. Checks for the leaf condition.** If there is exactly one primitive, a leaf is made immediately with `InitLeaf`.

**3. Picks the split axis.** The centroid extents in x, y, and z are computed, and the widest axis is chosen:

```cpp
double xdiff = xmax - x, ydiff = ymax - y, zdiff = zmax - z;
int dim;
if      (xdiff >= ydiff && xdiff >= zdiff) dim = 0;
else if (ydiff >= xdiff && ydiff >= zdiff) dim = 1;
else                                        dim = 2;
```

This ensures splits happen along the axis where primitives are most spread out, keeping children spatially separated.

**4. Degeneracy guard.** If all centroids are identical on the chosen axis (`bmax[dim] == bmin[dim]`), splitting is impossible — a leaf is returned for the whole range regardless of primitive count.

**5. Runs SAH to find the split point** (see next section). SAH partitions `hittableinfo[start..end]` in-place and returns the `mid` index.

**6. Recurses** into `[start, mid)` and `[mid, end)`, then wires the two resulting nodes together as an interior node with `InitInterior`:

```cpp
void InitInterior(int axis, BVHbuildnode *c0, BVHbuildnode *c1) {
    children[0] = c0;
    children[1] = c1;
    bounds = boundUnion(c0->bounds, c1->bounds);
    splitAxis = axis;
    nhittables = 0;  // 0 signals interior node
}
```

After the tree is built, the `hittables` vector is swapped with `orderedprims` assembled during leaf creation, so that leaf node offsets refer to contiguous, cache-friendly ranges in the final primitive array.

---

### Surface Area Heuristic (SAH)

**File:** `src/bvhsah.cpp`

SAH is the gold standard for BVH split quality. It models the expected cost of a ray intersection as:

```
cost(split) = t_traversal + (SA(left)/SA(parent)) * N_left
                           + (SA(right)/SA(parent)) * N_right
```

where SA is surface area and N is primitive count. A smaller child surface area means fewer rays are likely to enter it, so the expected cost is lower.

**Small-range fast path.** When there are 4 or fewer primitives, SAH bucket overhead isn't worth it. Insertion sort is used instead to sort by centroid on the split axis, and the midpoint index is returned directly.

**Bucket partitioning (8 buckets).** For larger ranges, the centroid span on the chosen axis is divided into 8 equal buckets. Each primitive is assigned to a bucket:

```cpp
int b = 8 * (centroid[dim] - centroidbounds.bmin[dim]) / centroidboundslength;
```

Each bucket accumulates a count and a bounding box union of all primitives that fall into it.

**Cost evaluation.** For each of the 7 possible split planes between 8 buckets, the algorithm sweeps left-to-right and right-to-left to accumulate the surface area and count for each partition:

```cpp
cost[i] = 0.125f
         + (count0 * b0.surfaceArea() + count1 * b1.surfaceArea())
           / bounds.surfaceArea();
```

The 0.125 constant is the traversal cost — the relative expense of stepping through an interior node versus directly testing a primitive.

**Leaf vs. split decision.** The minimum bucket cost is compared against `leafCost = nhittables` (cost of a flat leaf). If splitting costs more than not splitting, a leaf is returned — the tree naturally stops subdividing when further splitting is wasteful:

```cpp
if (nhittables > maxPrimsPerNode || minCost < leafCost)
    // partition and recurse
else
    // make a leaf
```

**Partition step.** Once the winning bucket index is known, `hittableinfo[start..end]` is partitioned in-place with a single scan using `std::swap`, so all primitives in buckets `<= minCostSplitBucket` end up before `mid` with no extra allocation.

---

### Memory Arena

**File:** `include/memoryArena.h`

All `BVHbuildnode`s during the build phase are allocated from a single `MemoryArena` — a monotonic bump allocator backed by one large `operator new` call at construction:

```cpp
void* allocate(size_t size, size_t alignment) {
    std::byte *current = buffer + offset;
    size_t space = capacity - offset;
    void* aligned_ptr = current;
    std::align(alignment, size, aligned_ptr, space); // handles alignment padding
    offset = (std::byte*)aligned_ptr - buffer + size;
    return aligned_ptr;
}
```

This eliminates per-node heap allocation overhead during the recursive build, which for large scenes would otherwise cause thousands of small `malloc` calls and fragmentation. The arena is destroyed in one shot when the build completes. `std::align` ensures each node lands at a properly aligned address.

---

### Phase 2 — Flatten: Tree to Linear Array

**File:** `src/BVHFlatten.cpp`

The pointer-based `BVHbuildnode` tree is not traversal-friendly — pointer chasing hurts cache performance. `flattenBVHTree` converts it into a pre-order `LinearBVHNode` array:

```cpp
struct LinearBVHNode {
    Bounds3f bounds;
    union {
        int hittablesOffset;   // leaf: index into ordered primitives
        int secondChildOffset; // interior: index of right child
    };
    int nhittables;  // 0 = interior node
    int axis;
    uint8_t pad[4];  // explicit padding to avoid cache-line straddling
};
```

The layout is designed so that the left child of any interior node is always the very next element in the array (`currentNodeIndex + 1`). Only the right child needs an explicit offset stored. Taking the left branch during traversal costs nothing — the traverser just increments its index.

The explicit 4-byte `pad` field keeps each `LinearBVHNode` within a predictable cache line boundary.

`flattenBVHTree` is a pre-order DFS:

```cpp
int BVHAccelerator::flattenBVHTree(BVHbuildnode *node, int *offset) {
    LinearBVHNode *linearnode = &nodes[*offset];
    linearnode->bounds = node->bounds;
    int newoffset = (*offset)++;

    if (node->nhittables > 0) {                       // leaf
        linearnode->hittablesOffset = node->firstoffset;
        linearnode->nhittables = node->nhittables;
    } else {                                           // interior
        linearnode->axis = node->splitAxis;
        linearnode->nhittables = 0;
        flattenBVHTree(node->children[0], offset);    // left child sits at offset+1
        linearnode->secondChildOffset =
            flattenBVHTree(node->children[1], offset);
    }
    return newoffset;
}
```

After flattening, the arena and all `BVHbuildnode`s are discarded. Only the flat `LinearBVHNode[]` array and the reordered primitives vector are kept for rendering.

---

### Phase 3 — Traverse: Iterative Ray Intersection

**File:** `include/hittable_list.h` — `hitBVH()`

Traversal is fully iterative using a fixed-size stack (`nodesToVisit[64]`), avoiding function-call overhead of recursion:

```cpp
int toVisitOffset = 0, currentNodeIndex = 0;
int nodesToVisit[64];

while (true) {
    const LinearBVHNode *node = &linearnode[currentNodeIndex];

    if (r.intersectBounds(node->bounds)) {
        if (node->nhittables > 0) {
            // Leaf: test all primitives in this node's range
            for (int i = start; i < end; i++)
                objects[i]->hit(r, interval(ray_t.mint, closest_t), temp_rec);
            if (toVisitOffset == 0) break;
            currentNodeIndex = nodesToVisit[--toVisitOffset];
        } else {
            // Interior: push far child, step into near child
            if (r.direction()[node->axis] < 0) {
                nodesToVisit[toVisitOffset++] = currentNodeIndex + 1;
                currentNodeIndex = node->secondChildOffset;
            } else {
                currentNodeIndex++;
                nodesToVisit[toVisitOffset++] = node->secondChildOffset;
            }
        }
    } else {
        // Bounds miss: pop stack
        if (toVisitOffset == 0) break;
        currentNodeIndex = nodesToVisit[--toVisitOffset];
    }
}
```

**Near-child ordering.** When descending into an interior node, the traverser checks the sign of the ray direction along the node's split axis. The child whose bounding box is spatially closer to the ray origin is visited first. Since `closest_t` shrinks with every successful hit, the near child is more likely to yield a valid intersection that lets the far child's entire subtree be culled.

---

### Ray–AABB Intersection

**File:** `include/ray.h` — `intersectBounds()`

Each node's bounds are tested with the standard slab method: for each axis, compute the ray entry and exit t-values using the reciprocal of the ray direction, intersect the three intervals, and check if the resulting interval is non-empty and overlaps the current valid ray window.

---

## Thread Pool

**Files:** `include/threadpool.h`, `src/threadpool.cpp`

### Design

The pool is a fixed-size worker pool — N threads are created once at construction and wait on a condition variable until tasks arrive. The task queue holds `std::function<void()>` objects, which lets any callable (lambdas, bound functions) be enqueued without the queue needing to know their signature.

`enqueue` is a variadic template that accepts any callable and its arguments, returning a `std::future` of the correct return type:

```cpp
template<typename F, typename... Args>
auto ThreadPool::enqueue(F&& f, Args&&... args)
    -> std::future<decltype(f(args...))>
{
    // Bind args into a zero-argument callable
    auto func = std::bind(std::forward<F>(f), std::forward<Args>(args)...);

    // Wrap in packaged_task for future/promise wiring
    auto encapsulated_ptr =
        std::make_shared<std::packaged_task<decltype(f(args...))()>>(func);

    std::future<std::result_of_t<F(Args...)>> future_object =
        encapsulated_ptr->get_future();
    {
        std::unique_lock<std::mutex> lock(mutex);
        queue.emplace([encapsulated_ptr]() { (*encapsulated_ptr)(); });
    }
    cv.notify_one();       // wake exactly one sleeping worker
    return future_object;  // caller can .get() to block until done
}
```

`std::packaged_task` is captured by `shared_ptr` inside the lambda so the task object stays alive until after it executes, even if `enqueue`'s stack frame has long returned. The lambda is type-erased to `std::function<void()>` — this is what lets tasks of different return types coexist in the same `std::queue`.

The pool is non-copyable and non-movable (explicitly deleted copy/move constructors and assignment operators), since workers hold a pointer to the pool's own mutex and condition variable.

### Tile-Based Rendering

Rather than assigning one row or one pixel per task, the renderer divides the image into **32×32 pixel tiles** and submits one task per tile:

```cpp
for (int i = 0; i < image_height; i += tileDimension) {
    for (int j = 0; j < image_width; j += tileDimension) {
        futures.push_back(pool.enqueue([this, i, j, &world] {
            fillTile(i, j, world);
        }));
    }
}
```

`fillTile` computes all `samples_per_pixel` samples for every pixel in its tile and writes results into a pre-allocated flat `writebuffer` (`std::vector<color>` of size `image_width * image_height`). Because each tile writes to a disjoint region of the buffer, there are no data races and no locking is needed inside `fillTile`.

After all futures are collected, the main thread calls `.get()` on each to drain completion, then writes the buffer to stdout in scanline order. Tile granularity is a tuning choice: tiles large enough to amortise task submission overhead, small enough to keep all threads busy throughout — avoiding the straggler problem common with row-per-task designs.

### Lifecycle

**Construction:** `stop = false`, then N worker threads are launched, each running `worker()`. N defaults to `std::thread::hardware_concurrency()`.

**`worker()` loop:**

```cpp
void ThreadPool::worker() {
    for (;;) {
        std::function<void()> cur_task;
        {
            std::unique_lock<std::mutex> lock(mutex);
            cv.wait(lock, [this]() { return stop || !queue.empty(); });
            if (stop && queue.empty()) break;
            cur_task = queue.front();
            queue.pop();
        }
        cur_task();   // execute outside the lock
    }
}
```

The lock is released before executing the task so other workers can dequeue concurrently. The predicate `stop || !queue.empty()` guards against spurious wakeups.

**Destruction:** The destructor sets `stop = true` under the lock, then calls `cv.notify_all()` to wake every sleeping worker. Workers that wake with an empty queue and `stop == true` exit their loop. The destructor then joins all threads, ensuring no work is abandoned mid-flight.

---

## Benchmarks

All benchmarks used 100 samples per pixel, max depth 50, and a scene of ~500 spheres.

| Configuration                           | Width | Time (s)  |
|-----------------------------------------|-------|-----------|
| Naive (no BVH, single thread)           | 300   | 235.7     |
| BVH only (single thread)                | 300   | 6.6       |
| Multithreading only, 16×16 tiles        | 300   | 37.2      |
| **Multithreading + BVH, 16×16 tiles**   | 300   | **3.2**   |
| BVH only (single thread)                | 1200  | 101.5     |
| Multithreading + BVH, 16×16 tiles       | 1200  | 51.1      |
| **Multithreading + BVH, 32×32 tiles**   | 1200  | **50.1**  |

BVH alone delivers a ~36× speedup over naive intersection. Multithreading on top gives a further ~2× at 1200px width, for a combined ~73× speedup over the single-threaded naive baseline.

---

## Project Structure

```
Ray_Tracer/
├── include/
│   ├── bvh_accelerator.h     # BVHAccelerator class declaration
│   ├── bvh_commons.h         # BVHhittableInfo, BVHbuildnode, LinearBVHNode
│   ├── bounds.h              # Bounds3f AABB, boundUnion
│   ├── threadpool.h          # ThreadPool class + enqueue template
│   ├── camera.h              # Camera, tile-based render loop, fillTile
│   ├── memoryArena.h         # Bump allocator for BVH build phase
│   ├── hittable.h            # Abstract hittable base + hit_record
│   ├── hittable_list.h       # Scene container + hitBVH iterative traversal
│   ├── sphere.h              # Sphere primitive + bounds calculation
│   ├── material.h            # Lambertian, metal, dielectric
│   ├── ray.h                 # Ray + intersectBounds (slab method)
│   ├── vec3.h                # Vector math
│   ├── color.h               # Color write + gamma correction
│   └── interval.h            # [min, max] interval helpers
├── src/
│   ├── main.cpp              # Scene setup, BVH construction, render call
│   ├── bvh_accelerator.cpp   # BVHAccelerator constructor
│   ├── bvhrecursivebuild.cpp # recursiveBuild — axis selection, SAH dispatch
│   ├── bvhsah.cpp            # SAH bucket evaluation + in-place partition
│   ├── BVHFlatten.cpp        # flattenBVHTree — pointer tree → linear array
│   └── threadpool.cpp        # Worker loop, constructor, destructor
├── output/image.ppm          # Sample render output
├── benchmark_results.csv     # Recorded timing data
└── makefile                  # g++ C++17, -Iinclude
```
