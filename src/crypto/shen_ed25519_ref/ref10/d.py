q = 2**255 - 19

def expmod(b,e,m):
  if e == 0: return 1
  t = expmod(b,e/2,m)**2 % m
  if e & 1: t = (t*b) % m
  return t

def inv(x):
  return expmod(x,q-2,q)

def radix255(x):
  x = x % q
  if x + x > q: x -= q
  x = [x,0,0,0,0,0,0,0,0,0]
  bits = [26,25,26,25,26,25,26,25,26,25]
  for i in range(9):
    carry = (x[i] + 2**(bits[i]-1)) / 2**bits[i]
    x[i] -= carry * 2**bits[i]
    x[i + 1] += carry
  result = ""
  for i in range(9):
    result = result+str(x[i])+","
  result = result+str(x[9])
  return result

d = -121665 * inv(121666)
print radix255(d)
