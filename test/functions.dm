print(add(4.0, 6.0))
print(div(7.0, 8.0))
print(identity(5.0))
print(identity(false))
print(identity(64))
print(sub(7, 4))

function add(a, b) a + b
function div(a, b) a / b
function identity(x) x

function sub(a, b)
    a - b

--- Output
10
0.875
5
false
64
3
---
