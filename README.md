CRC LIRS Plus
===
PA for computer architecture course

A smart threshold adjustment optimization is applied to traditional LIRS algorithm

A huge improvement is observed using my benchmark as shown below

| Algorithm | LLC miss time |
| --- | --- |
|LRU | 				238161 |
| L2LRU	|		239892 |
| L2LRU+Est	|	239940 |
| Stride0.9	|	238170 |
| DBP	|			229554 |
| LIRS	|		237246 |
| LIRSplus	|	207601 |
| RLIRS	|		259870 |



