import asyncdispatch
import asyncfutures

# Each future may only be awaited by one async proc, and the async
# dispatcher will run all ready tasks on poll(). If we want to run
# tasks one at a time, we should keep a sequence of Futures.
var futures = newSeq[Future[void]]()

# This proc will act like a stopping point in an async proc. With
# a task manager, it behaves like cooperative multitasking.
proc nextFrame*(): Future[void] =
  echo "nextFrame"
  var f = newFuture[void]("frame")
  futures.add(f)
  return f

# Continues last paused async proc. Use a queue for FIFO behavior.
proc popFuture*() =
  # should really check size first...
  let f = futures.pop()
  f.complete()

proc bar() {.async.} =
  echo "bar"
  await nextFrame()
  echo "bar done"

proc foo*() {.async.} =
  echo "foo"
  await nextFrame()
  echo "foo done"

# unlike Python, you can simply invoke async functions anywhere 
discard foo() #> foo
discard bar() #> bar

echo "pop"
popFuture() #> bar done
echo "pop2"
popFuture() #> foo done
