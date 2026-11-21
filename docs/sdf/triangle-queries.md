# Mesh-local triangle queries

**English** | [简体中文](triangle-queries.zh-CN.md)

`TriangleMeshQueries` owns a mesh handle and one immutable spatial index. The constructor accepts a spatial-query provider; the convenience constructor uses the CPU provider. Each indexed bound encloses one referenced face and retains its zero-based source face ID.

`Closest` returns a mesh-local surface point, unsigned Euclidean distance in metres, and face ID. The default provider resolves equal distances by the smallest ID. Face interiors, edges and vertices participate. `RayHits` reports forward hits ordered by parameter and then face ID. The parameter multiplies the supplied direction. Face-interior, edge, vertex and coplanar hits are distinguished; shared boundaries can produce several face records at one parameter. Coplanar rays report the first forward intersection with each intersected triangle. Crossing parity belongs to solid classification.

Coordinates and directions must be finite, and ray directions must be nonzero. The leaf test uses scaled double-precision arithmetic and a 32-epsilon barycentric roundoff window for edge classification. Unrepresentable computations can raise `std::overflow_error`.
