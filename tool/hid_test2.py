import hid
devs = hid.enumerate()
print('Count:', len(devs))
for d in devs:
    print(d)
