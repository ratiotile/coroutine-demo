# Nim coroutines benchmark v3 try to reduce refs to speed up
import asyncdispatch
import asyncfutures
import deques
import math
import times
import circular_queue

const
  FRAMES = 2 # future frames to keep
  DISTANCE_TO_MINE = 10
  WORKER_SPEED = 1
  WORKER_CAPACITY = 1
  FRAMES_TO_MINE = 10
  NUM_TASKS = 1000
  FRAMES_TO_RUN = 1000000

type
  World = ref object
    total_mined: int
    workers: array[NUM_TASKS, Worker]
  Worker = object
    carrying: int
    position: int
    mining_progress: int
    ticks: int
    total_mined: int

proc newWorker(): Worker =
  result = Worker(
    carrying: 0, 
    position: 0, 
    mining_progress: 0,
    ticks: 0,
    total_mined: 0,
  )

proc gather(this: var Worker) =
  this.ticks += 1
  this.mining_progress += 1
  if(this.mining_progress >= FRAMES_TO_MINE):
    this.carrying = WORKER_CAPACITY
    this.mining_progress = 0

proc isMining(this: Worker): bool =
  return this.mining_progress > 0

proc dropoff(this: var Worker) =
  this.total_mined += this.carrying
  this.carrying = 0

proc moveToMine(this: var Worker) =
  this.ticks += 1
  this.position += WORKER_SPEED

proc moveToHome(this: var Worker) =
  this.ticks += 1
  this.position -= WORKER_SPEED

proc isAtMine(this: Worker): bool =
  return this.position >= DISTANCE_TO_MINE

proc isAtHome(this: Worker): bool =
  return this.position <= 0

type 
  TaskManager* = object
    frame_futures: CircularQueue[FRAMES, FutureQueue]
    current_frame*: int
    world: World
    tasks: seq[Task]
  # Fix the immediate execution of tasks, and associate data with an object
  Task = object
    # name: string
    worker: ptr Worker

  FutureT = Future[void]
  FutureQueue = seq[FutureT]
  #FutureQueue = CircularQueue[NUM_TASKS, FutureT]

# create a new TaskManager with futures preallocated
proc newTaskManager*(): TaskManager =
  var tm = TaskManager(
    frame_futures: newCircularQueue[FRAMES, FutureQueue](),
    current_frame: 1,
    world: World(total_mined: 0)
  )
  for i in 1..FRAMES:
    tm.frame_futures.addLast(newSeq[FutureT]())
    #tm.frame_futures.addLast(newCircularQueue[NUM_TASKS, FutureT]())
  return tm

# suspend async proc for a number of frames before running again
proc frame*(this: var TaskManager, frames = 1): FutureT =
  var f = newFuture[void]("frame")
  # index 0 is current frame
  this.frame_futures[frames].add(f)
  return f

# removes the current frame, adding a replacement
proc popFrame*(this: var TaskManager) =
  #this.frame_futures.popFirst()
  this.frame_futures.shift()
  #this.frame_futures.addLast(newCircularQueue[NUM_TASKS, FutureT]())
  #this.frame_futures[FRAMES - 1].clear()
  this.frame_futures[FRAMES - 1].setLen(0)
  this.current_frame += 1

# execute one future
proc step(this: var TaskManager) = 
  if this.frame_futures.peekFirst().len() == 0:
    this.popFrame()
  else:
    this.frame_futures[0].pop().complete()

proc doFrame(this: var TaskManager) =
  while this.frame_futures.peekFirst().len() > 0:
    this.frame_futures[0].pop().complete()
  this.popFrame()

var task_id = 0
var tm: TaskManager

proc nextTaskId(): int =
  task_id += 1
  return task_id

proc goToMine(this: Task) {.async.} =
  while not this.worker[].isAtMine():
    this.worker[].moveToMine()
    await tm.frame()

proc doMining(this: Task) {.async.} =
  while true:
    this.worker[].gather()
    await tm.frame()
    if not this.worker[].isMining():
      break

proc dropoff(this: Task) {.async.} =
  while not this.worker[].isAtHome():
    this.worker[].moveToHome()
    await tm.frame()
  this.worker[].dropoff()

proc run(this: Task) {.async, gcsafe.}=
  while true:
    await this.goToMine()
    await this.doMining()
    await this.dropoff()

# creates new task internally, don't see a reason why not
proc addTask*(this: var TaskManager, worker: ptr Worker) =
  var task = Task(
    # name: "Task " & $nextTaskId(),
    # tm: unsafeAddr this,
    worker: worker,
  )
  var cb =  proc(future: FutureT) {.closure, gcsafe.} =
    discard task.run()
  var f = newFuture[void]("starter")
  f.callback = cb
  this.frame_futures[0].add(f)



tm = newTaskManager()

for i in 1..NUM_TASKS:
  let worker = newWorker()
  tm.world.workers[i-1] = worker
  tm.addTask(unsafeAddr worker)

let start_t = getTime()
for i in 1..FRAMES_TO_RUN:
  #tm.doFrame()
  tm.step()
let end_t = getTime()
let diff = end_t - start_t

tm.world.total_mined = 0
for i in 0..NUM_TASKS-1:
  tm.world.total_mined += tm.world.workers[i].total_mined

echo "Mined a total of " & $tm.world.total_mined & " took " & $diff
echo "ticks: " & $tm.world.workers[0].ticks
# @1000/1000, Mined a total of 33000 took 164 milliseconds, 9 microseconds, and 400 nanoseconds
# @1000/1000000 stepping, Mined a total of 33000 took 7 seconds, 507 milliseconds, 429 microseconds, and 400 nanoseconds
# about 7.5 microseconds per step
# v0.20
# @1000/1000000 stepping, Mined a total of 33000 took 6 seconds, 127 milliseconds, 268 microseconds, and 600 nanoseconds
# about 6127 ns per step
# Circular Queues improved it to 7110903000ns @1000/1000 frames
# and 7084399600 @1000/1000000 steps, seems to be worse
# switching back to seq for futures @1000/1000000 step: 6122277400, about the same
# with the asserts compiled out: 6095274000 ns, slightly better
# let's try switching the inner seqs back to circular arrays.
# @1000/1000000 step: 7010390200 nc, which isn't that much better than before
# Lets try removing refs and using unsafe ptrs @1000/1000000
# Workers to ptr: 6042767400 ns, slightly better
# Global TaskManager: 5956756400 ns
# non-ref world made things slightly worse
# Task non-ref, removed RootObj: 5930753100

# in contrast, C++ duff's device coroutines took 0.293494 seconds (unoptimized), or 0.3 microseonds per step.
# C++ MiLi coroutines are 30x faster (unoptimized)
# optimized @1000/1000000 stepping, 15372180 ns = 15ns per step, or 500x faster?
#[ C++
Release completes 33000 trips in 20ns per step (300x faster)

]#

