## Data Accessing by Probabilistic Filters and Multiway Trees

* Probabilistic Filters

  * Bloom Filter *(murmurhash3)*

  * Cuckoo Filter *(murmurhash3, rabin fingerprint)*
    * Low load factor implementation *(memory efficiency tradeoff)*
    * High load factor (~100%) implementation *(lookup and deletion time tradeoff)*

* Multiway Trees

  * B Tree (inspired by [CalebLBaker](https://github.com/CalebLBaker/b-tree))
    * A self-balancing tree data structure where each node has multiple keys and children

  * B+ Tree
    * Memory based implementation (inspired by [shashikdm](https://github.com/shashikdm/B-Plus-Tree))
    * Disk based implementation (inspired by [aayushmudgal](https://github.com/aayushmudgal/bplustree))
