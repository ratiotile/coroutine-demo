# Nim coroutines benchmark v4 simplify to stepping only
import asyncdispatch
import asyncfutures
import math
import times
import circular-queue/circular_queue

const
  DISTANCE_TO_MINE = 10
  WORKER_SPEED = 1
  WORKER_CAPACITY = 1
  FRAMES_TO_MINE = 10
  NUM_TASKS = 1000
  FRAMES_TO_RUN = 1000000

type
  Worker = ref object
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

proc gather(this: Worker) {.gcsafe.}=
  this.ticks += 1
  this.mining_progress += 1
  if(this.mining_progress >= FRAMES_TO_MINE):
    this.carrying = WORKER_CAPACITY
    this.mining_progress = 0

proc isMining(this: Worker): bool {.gcsafe.} =
  return this.mining_progress > 0

proc dropoff(this: Worker) {.gcsafe.} =
  this.total_mined += this.carrying
  this.carrying = 0

proc moveToMine(this: Worker) {.gcsafe.} =
  this.ticks += 1
  this.position += WORKER_SPEED

proc moveToHome(this: Worker) {.gcsafe.} =
  this.ticks += 1
  this.position -= WORKER_SPEED

proc isAtMine(this: Worker): bool {.gcsafe.} =
  return this.position >= DISTANCE_TO_MINE

proc isAtHome(this: Worker): bool {.gcsafe.} =
  return this.position <= 0

type 
  TaskManager* = object
    frame_futures: CircularQueue[NUM_TASKS, FutureT]
    tasks: array[NUM_TASKS, Task]
  # Fix the immediate execution of tasks, and associate data with an object
  Task = ref object
    worker: Worker
    #id: int

  FutureT = Future[void]

# create a new TaskManager with futures preallocated
proc newTaskManager*(): TaskManager =
  var tm = TaskManager(
    frame_futures: newCircularQueue[NUM_TASKS, FutureT](),
  )
  return tm

# suspend async proc until next step
proc frame*(this: var TaskManager): FutureT {.gcsafe.}=
  var f = newFuture[void]("frame")
  # index 0 is current frame
  this.frame_futures.add(f)
  return f

# execute one future
proc step(this: var TaskManager) = 
  let f = this.frame_futures.pop()
  f.complete()

#var task_id = 0
var tm: TaskManager

#proc nextTaskId(): int =
#  task_id += 1
#  return task_id

proc goToMine(this: Task) {.async, gcsafe.} =
  while not this.worker.isAtMine():
    this.worker.moveToMine()
    await tm.frame()

proc doMining(this: Task) {.async, gcsafe.} =
  while true:
    this.worker.gather()
    await tm.frame()
    if not this.worker.isMining():
      break

proc dropoff(this: Task) {.async, gcsafe.} =
  while not this.worker.isAtHome():
    this.worker.moveToHome()
    await tm.frame()
  this.worker.dropoff()

proc run(this: Task) {.async, gcsafe.}=
  while true:
    await this.goToMine()
    await this.doMining()
    await this.dropoff()

# creates new task internally, don't see a reason why not
proc addTask*(this: var TaskManager, i: int) =
  this.tasks[i] = Task(
    worker: newWorker(),
    #id: nextTaskId(),
  )
  let task = this.tasks[i]
  var cb =  proc(future: FutureT) {.closure, gcsafe.} =
    discard task.run()
  var f = newFuture[void]("starter")
  f.callback = cb
  this.frame_futures.add(f)

tm = newTaskManager()

for i in 0..NUM_TASKS-1:
  tm.addTask(i)

let start_t = getTime()
for i in 1..FRAMES_TO_RUN:
  tm.step()
let end_t = getTime()
let diff = end_t - start_t

var total_mined = 0
for i in 0..NUM_TASKS-1:
  total_mined += tm.tasks[i].worker.total_mined

echo "Mined a total of " & $total_mined & " took " & $diff
echo "ticks: " & $tm.tasks[0].worker.ticks
#[ C++
  Release completes 33000 trips in 20ns per step (300x faster)
# Previous best, switched step condition to most common first:
#   5916251300 ns = 5916ns per step
  Now much faster! 2d arrays make quite a difference!
    98005600 ns = 98ns per step, 60x faster! now only 5x slower than C++
  Removed task ids (for debugging):
    90005300 ns = 90ns per step, 10% improvement!
    52003000 ns (gc off)
  Tasks as refs:
    110006200 ns: bit slower
  Tasks and TaskManager as refs:
    95003400 ns : not bad
  Everything as refs (+workers):
    94005400 ns : surprisingly good
    106006100ns : actually GC-safe, can't have TaskManager as ref!
    126016000ns : checks on, 600% slower than C++
]#

