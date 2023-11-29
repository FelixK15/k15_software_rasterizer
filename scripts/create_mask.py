
def create8xShuffleBitMaskLUT():
    for i in range(256):
        output = 0
        outputShift = 21

        for j in range(8):
            if( i & (1<<j) > 0 ):
                output |= j << outputShift
                outputShift -= 3
        
        print('0b{:024b}'.format(output))

def create4xShuffleBitMaskLUT():
    for i in range(16):
        output = 0
        outputShift = 6

        for j in range(4):
            if( i & (1<<j) > 0 ):
                output |= j << outputShift
                outputShift -= 2
        
        print('0b{:08b}'.format(output))

print("4x shuffle mask:")
create4xShuffleBitMaskLUT()
print("=================")
print("8x shuffle mask:")
create8xShuffleBitMaskLUT()
print("=================")