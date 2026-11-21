# Metadata-aware LBVH traversal

**English** | [简体中文](metadata-pruning.zh-CN.md)

`FilteredMortonPlugin()` registers an optional filtered overlap provider. Each leaf carries two opaque caller tags, owner and category. A node records a uniform summary only when all leaves below it carry the same tags. The query predicate takes two metadata values and must be deterministic and side-effect free. Returning false for a uniform subtree skips every leaf in it. Mixed nodes descend and leaves receive the exact policy.

An individual box query uses the supplied source ID's metadata and includes its own ID when the box and predicate permit. Self-pairs appear once, in increasing ID order, with the lower ID's metadata as the first argument. The unfiltered methods ignore tags. The baseline loads query metadata at each predicate evaluation. Per-call counters report visits, query metadata reads, leaf tests and pruned internal nodes. Each query owns its counters and temporary storage.
