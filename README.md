## Data Accessing by Probabilistic Filters and Multiway Trees

* Bloom Filter *(murmurhash3)*

* Cuckoo Filter *(murmurhash3, rabin fingerprint)*
  * Low load factor implementation *(memory efficiency tradeoff)*
  * High load factor implementation *(lookup and deletion time tradeoff)*

* B Tree (inspired by [CalebLBaker](https://github.com/CalebLBaker/b-tree))
  * A self-balancing tree data structure where each node has multiple keys and children

* B+ Tree (inspired by [aayushmudgal](https://github.com/aayushmudgal/bplustree))
  * Disk based implementation of B+ Tree where leaf nodes denote actual data pointers to the disk
