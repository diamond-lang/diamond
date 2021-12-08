function is_odd(n)
    if n / 2.0 == 0.0 false
    else            true

function is_odd2(n)
    if n / 2.0 == 0.0
        false
    else
        true

function is_odd3(n) if n / 2.0 == 0.0 false
                    else            true

print(is_odd(4.0))
print(is_odd(7.0))

print(is_odd2(4.0))
print(is_odd2(7.0))

print(is_odd3(4.0))
print(is_odd3(7.0))

--- Output
true
false
true
false
true
false
---