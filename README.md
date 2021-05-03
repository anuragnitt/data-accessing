## Probabilistic Filters

* Bloom Filter *(murmurhash3)*

* Cuckoo Filter *(murmurhash3, rabin fingerprint)*
  * Low load factor implementation *(memory efficiency tradeoff)*
  * High load factor implementation *(lookup and deletion time tradeoff)*

## B-Tree

* A self-balancing tree data structure where each node has multiple keys and children
(inspired by [CalebLBaker](https://github.com/CalebLBaker/b-tree))
