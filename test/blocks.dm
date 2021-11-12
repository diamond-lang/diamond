function add20(a)
    twenty be 20
    return a + 20

function add10(a)
    ten be 10
    return add20(a) - ten

function say23()
    print(23)
    return

function say76()
    print(76)

print(add10(50))
print(add20(50))
say23()
say76()

--- Output
60
70
23
76
---