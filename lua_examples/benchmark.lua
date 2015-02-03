function runOnce(freq)
sum = 0
node.set_cpu_freq(freq)
for runs=0,4 do

  now=tmr.now()
  a = 0
  b = 2
  c = 1
for ii=0,10 do
  for i=0,1000 do
    a = i+b
    c = bit.bxor(c, 1)
    tmr.wdclr()
    node.set_cpu_freq(freq)
  end
end

  nnow = tmr.now()
  timeDiff = (nnow-now)/1000
  sum = sum + timeDiff
  
  print("time needed: " .. timeDiff )
end
print("total time needed: " .. sum)
end


print("======================")
freq=node.setcpufreq(80)
print("Setting CPU FREQUENCY TO " .. freq .. "MHz")
runOnce(80)
print("======================")

print("======================")
freq=node.setcpufreq(160)
print("Setting CPU FREQUENCY TO " .. freq .. "MHz")
runOnce(160)
print("======================")



