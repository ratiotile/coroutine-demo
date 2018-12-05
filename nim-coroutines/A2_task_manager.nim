import asyncdispatch
import asyncfutures
import deques
import math

const
  FRAMES = 5 # future frames to keep

type 
  TaskManager = object of RootObj
    frame_futures: Deque[seq[Future[void]]]

# create a new TaskManager with futures preallocated
proc newTaskManager(): TaskManager =
  var tm = TaskManager(frame_futures: initDeque[seq[Future[void]]](nextPowerOfTwo(FRAMES)))
  for i in 1..5:
    tm.frame_futures.addLast(@[])
  return tm

# suspend async proc for a number of frames before running again
proc frame*(this: var TaskManager, frames = 1): Future[void] =
  var f = newFuture[void]("frame")
  # index 0 is current frame
  this.frame_futures[frames].add(f)
  return f

# runs all waiting tasks, then removes the current frame
proc popFrame*(this: var TaskManager) =
  var current_futures = this.frame_futures[0]
  while current_futures.len() > 0:
    current_futures.pop().complete()
  this.frame_futures.popFirst()
  this.frame_futures.addLast(@[])

var tm = newTaskManager()

proc bar() {.async.} =
  echo "bar 0"
  await tm.frame()
  echo "bar 1"
  await tm.frame()
  echo "bar 2"
  await tm.frame()
  echo "bar 3"
  await tm.frame()
  echo "bar 4"
  await tm.frame()
  echo "bar 5"

proc foo() {.async.} =
  echo "foo 0"
  await tm.frame(2)
  echo "foo 2"
  await tm.frame(3)
  echo "foo 5"

# unlike Python, you can simply invoke async functions anywhere 
discard foo() #> foo 0
discard bar() #> bar 0

tm.popFrame() # first frame is empty
for i in 1..5:
  echo "frame " & $i
  tm.popFrame()
#> frame 1
#> bar 1
#> frame 2
#> bar 2
#> foo 2
#> frame 3
#> bar 3
#> frame 4
#> bar 4
#> frame 5
#> bar 5
#> foo 5