# Nim coroutines benchmark
import asyncdispatch
import asyncfutures
import deques
import math
import times

const
  FRAMES = 2 # future frames to keep
  DISTANCE_TO_MINE = 10
  WORKER_SPEED = 1
  WORKER_CAPACITY = 1
  FRAMES_TO_MINE = 10
  NUM_TASKS = 1000
  FRAMES_TO_RUN = 1000

type
  World = ref object of RootObj
    total_mined: int
  Worker = ref object of RootObj
    carrying: int
    position: int
    mining_progress: int
    world: World

proc newWorker(world: World): Worker =
  var w = Worker(
    carrying: 0, 
    position: 0, 
    mining_progress: 0,
    world: world)
  return w

proc gather(this: Worker) =
  this.mining_progress += 1
  if(this.mining_progress >= FRAMES_TO_MINE):
    this.carrying = WORKER_CAPACITY
    this.mining_progress = 0

proc isMining(this: Worker): bool =
  return this.mining_progress > 0

proc dropoff(this: Worker) =
  this.world.total_mined += this.carrying
  this.carrying = 0

proc moveToMine(this: Worker) =
  this.position += WORKER_SPEED

proc moveToHome(this: Worker) =
  this.position -= WORKER_SPEED

proc isAtMine(this: Worker): bool =
  return this.position >= DISTANCE_TO_MINE

proc isAtHome(this: Worker): bool =
  return this.position <= 0

type 
  TaskManager* = ref object of RootObj
    frame_futures: Deque[seq[Future[void]]]
    current_frame*: int
    world: World
  # Fix the immediate execution of tasks, and associate data with an object
  Task = ref object of RootObj
    name: string
    tm: TaskManager
    worker: Worker

# create a new TaskManager with futures preallocated
proc newTaskManager*(): TaskManager =
  var tm = TaskManager(
    frame_futures: initDeque[seq[Future[void]]](nextPowerOfTwo(FRAMES)),
    current_frame: 1,
    world: World(total_mined: 0)
  )
  for i in 1..FRAMES:
    tm.frame_futures.addLast(newSeqOfCap[Future[void]](NUM_TASKS))
  return tm

# suspend async proc for a number of frames before running again
proc frame*(this: TaskManager, frames = 1): Future[void] =
  var f = newFuture[void]("frame")
  # index 0 is current frame
  this.frame_futures[frames].add(f)
  return f

# removes the current frame, adding a replacement
proc popFrame*(this: TaskManager) =
  this.frame_futures.popFirst()
  this.frame_futures.addLast(@[])
  this.current_frame += 1

# execute one future
proc step(this: TaskManager) = 
  if this.frame_futures.peekFirst().len() == 0:
    this.popFrame()
  else:
    this.frame_futures[0].pop().complete()

proc doFrame(this: TaskManager) =
  while this.frame_futures.peekFirst().len() > 0:
    this.frame_futures[0].pop().complete()
  this.popFrame()
  
proc goToMine(this: Task) {.async.} =
  while not this.worker.isAtMine():
    this.worker.moveToMine()
    await this.tm.frame()

proc doMining(this: Task) {.async.} =
  while true:
    this.worker.gather()
    await this.tm.frame()
    if not this.worker.isMining():
      break

proc dropoff(this: Task) {.async.} =
  while not this.worker.isAtHome():
    this.worker.moveToHome()
    await this.tm.frame()
  this.worker.dropoff()

proc run(this: Task) {.async, gcsafe.}=
  while true:
    await this.goToMine()
    await this.doMining()
    await this.dropoff()

var task_id = 0
proc nextTaskId(): int =
  task_id += 1
  return task_id

# creates new task internally, don't see a reason why not
proc addTask*(this: TaskManager) =
  var task = Task(
    name: "Task " & $nextTaskId(),
    tm: this,
    worker: this.world.newWorker()
  )
  var cb =  proc(future: Future[void]) {.closure, gcsafe.} =
    discard task.run()
  var f = newFuture[void]("starter")
  f.callback = cb
  this.frame_futures[0].add(f)

let tm = newTaskManager()

for i in 1..NUM_TASKS:
  tm.addTask()

let start_t = getTime()
for i in 1..FRAMES_TO_RUN:
  tm.doFrame()
let end_t = getTime()
let diff = end_t - start_t

echo "Mined a total of " & $tm.world.total_mined & " took " & $diff
# @1000/1000, Mined a total of 33000 took 164 milliseconds, 9 microseconds, and 400 nanoseconds
# @1000/1000000 stepping, Mined a total of 33000 took 7 seconds, 507 milliseconds, 429 microseconds, and 400 nanoseconds
# about 7.5 microseconds per step
# in contrast, C++ duff's device coroutines took 0.293494 seconds (unoptimized), or 0.3 microseonds per step.
# C++ MiLi coroutines are 30x faster! (unoptimized)
# optimized @1000/1000000 stepping, 15372180 ns = 15ns per step, or 500x faster?