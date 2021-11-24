struct buf {
  int valid;   // has data been read from disk?
  int disk;    // does disk "own" buf?
  uint dev;    // device number
  uint blockno;// disk block number
  struct sleeplock lock;
  uint refcnt; //reference count
  struct buf *prev; // LRU cache list
  struct buf *next;
  uint timestamp;
  uchar data[BSIZE]; //payload

};

