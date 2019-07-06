#[ Fixed size queue that wraps around. Make sure that the start does not outrun
the end. Should perform better than sequences. Has one wasted spot due
to implementation: needed to tell full from empty. ]#
type 
  CircularQueue*[s: static int, T] = object 
    ## A queue with a fixed maximum size, implemented with an array
    data : array[s + 1, T]
    idx_start: int  ## index of first element in queue
    idx_end: int  ## index after the last element in queue

when defined(release):
  proc newCircularQueue[s: static int, T]: CircularQueue[s, T] {.noInit.} =
    ## Constructs a new circular queue with uninitialized data in release mode
    result.idx_start = 0
    result.idx_end = 0

else:
  proc newCircularQueue[s: static int, T]: CircularQueue[s, T] =
    ## Constructs a new circular queue with data initialized in debug mode
    result.idx_end = 0

#[ Generates the same code as below:
proc size[s:static int, T](this: CircularQueue[s, T]): int =
  if this.idx_end >= this.idx_start:
    result = this.idx_end - this.idx_start
  else:
    result = s - this.idx_start
    result += this.idx_end
]#
proc size(this: CircularQueue): int {.inline, nosideEffect.} =
  ## Current number of items in the queue
  if this.idx_end >= this.idx_start:
    result = this.idx_end - this.idx_start
  else:
    result = len(this.data) - this.idx_start
    result += this.idx_end

proc len(this: CircularQueue): int {.inline, nosideEffect.} =
  ## Alias for size()
  this.size()

proc capacity(this: CircularQueue): int {.inline, nosideEffect.} = 
  ## The maximum capcity this queue can hold
  len(this.data) - 1

proc isEmpty(this: CircularQueue): bool {.inline, nosideEffect.} =
  ## Efficient way to determine if queue is empty
  this.idx_start == this.idx_end

proc isFull(this: CircularQueue): bool {.inline, nosideEffect.} = 
  ## Check if the queue is full.
  this.size() >= this.capacity()

proc peek[s, T](this: CircularQueue[s, T]): T {.inline, nosideEffect.} =
  assert(not this.isEmpty(), "cannot peek on empty queue")
  result = this.data[this.idx_start]

proc nextIndex(this: CircularQueue, i: int): int {.inline, nosideEffect.} =
  ## Calculate next wraparound index
  (i + 1) mod (this.capacity() + 1)

proc pop[s, T](this: var CircularQueue[s, T]): T {.inline.} =
  ## Remove and return the first item. The data is not cleared until overwritten
  ## by later adds.
  assert(not this.isEmpty(), "cannot pop from empty circular queue")
  result = this.data[this.idx_start]
  this.idx_start = this.nextIndex(this.idx_start)

proc add[s, T](this: var CircularQueue[s, T], item: T) {.inline.} =
  ## Pushes an item onto the back of the queue.
  assert not this.isFull(), "queue is full"
  this.data[this.idx_end] = item
  this.idx_end = this.nextIndex(this.idx_end)
  assert(this.idx_end != this.idx_start, "circular queue overflow")

when isMainModule:
  import unittest
  echo "running tests..."
  suite "Circular Queue empty tests":
    setup:
      const cap = 1
      var q = newCircularQueue[cap, int]()

    test "initial size should be 0":
      check(q.size() == 0)
      check len(q) == 0

    test "initial index state":
      check q.idx_end == 0
      check q.idx_start == 0
    
    test "raises on peek when empty":
      expect AssertionError:
        discard q.peek()

    test "raises on pop when empty":
      expect AssertionError:
        discard q.pop()

    test "capacity":
      check q.capacity() == cap

    test "nextIndex":
      check q.nextIndex(0) == 1
      check q.nextIndex(1) == 0
  
  suite "Start with a single item":
    setup:
      const cap = 1
      var q = newCircularQueue[cap, int]()
      q.add(1)

    test "size should be 1":
      check q.len() == 1

    test "end should be advanced":
      check q.idx_end == 1
      check q.idx_start == 0
    
    test "peek":
      let x = q.peek()
      check x == 1
      check q.size() == 1
      check q.idx_start == 0
      check q.idx_end == 1
      check q.capacity() == cap

    test "pop":
      let x = q.pop()
      check x == 1
      check q.size() == 0
      check q.idx_end == 1
      check q.idx_start == 1
      check q.capacity() == cap
      expect AssertionError:
        discard q.pop()

    test "over capacity add":
      expect AssertionError:
        q.add(2)

    test "pop, then add again":
      discard q.pop()
      q.add(2)
      check q.size() == 1
      check q.capacity() == cap
      check q.idx_end == 0
      check q.idx_start == 1
      check q.peek() == 2
      expect AssertionError:
        q.add(3)

    test "pop, add, pop, add":
      check q.pop() == 1
      q.add(2)
      check q.pop() == 2
      check q.len() == 0
      q.add(3)
      check q.len() == 1
      check q.peek() == 3
      expect AssertionError:
        q.add(4)
