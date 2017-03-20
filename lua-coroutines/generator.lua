print("hello world")

function range(x)
  return coroutine.create(function()
    for i=0,x do
      coroutine.yield(i)
    end
    return false
  end)
end

y = range(5)
repeat
  s, i = coroutine.resume(y)
  print(i)
until coroutine.status(y) == 'dead'
