function add20(a)
    twenty be 20
    return a + 20

function add10(a)
    ten be 10
    return add20(a) - ten

print(add10(50))
print(add20(50))

--- Output
60
70
---