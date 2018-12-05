import asyncdispatch
import asyncfutures
import deques
import math

const
  FRAMES = 5 # future frames to keep

type 
  # is a ref so that Task can hold a ref to it
  TaskManager* = ref object of RootObj
    frame_futures: Deque[seq[Future[void]]]
    current_frame*: int
  # Fix the immediate execution of tasks, and associate data with an object
  Task = ref object of RootObj
    name: string
    step: int # frames to skip between runs
    tm: TaskManager # just to get the current frame

# create a new TaskManager with futures preallocated
proc newTaskManager*(): TaskManager =
  var tm = TaskManager(
    frame_futures: initDeque[seq[Future[void]]](nextPowerOfTwo(FRAMES)),
    current_frame: 1
  )
  for i in 1..FRAMES:
    tm.frame_futures.addLast(@[])
  return tm

# suspend async proc for a number of frames before running again
proc frame*(this: TaskManager, frames = 1): Future[void] =
  var f = newFuture[void]("frame")
  # index 0 is current frame
  this.frame_futures[frames].add(f)
  return f

# runs all waiting tasks, then cycles to the next frame
proc popFrame*(this: TaskManager) =
  var current_futures = this.frame_futures[0]
  while current_futures.len() > 0:
    current_futures.pop().complete()
  this.frame_futures.popFirst()
  this.frame_futures.addLast(@[])
  this.current_frame += 1
  
# needs to be gcsafe for starter closure
method run(this: Task) {.async, base, gcsafe.}=
  while true:
    echo this.name & $this.tm.current_frame
    await this.tm.frame(this.step)

# need to create a starter closure callback to run the task on popFrame()
proc addTask*(this: TaskManager, task: Task) =
  task.tm = this
  var f = newFuture[void]("starter")
  f.callback = proc(future: Future[void]) {.closure.} =
    discard task.run()
  this.frame_futures[0].add(f)

let tm = newTaskManager()
let foo = Task(name: "foo ", step: 1)
let bar = Task(name: "bar ", step: 2)
tm.addTask(foo)
tm.addTask(bar)

for i in 1..5:
  echo "frame " & $i
  tm.popFrame()
#> frame 1
#> bar 1
#> foo 1
#> frame 2
#> foo 2
#> frame 3
#> foo 3
#> bar 3
#> frame 4
#> foo 4
#> frame 5
#> foo 5
#> bar 5