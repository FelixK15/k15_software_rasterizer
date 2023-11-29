
def create8xShuffleBitMaskLUT():
    elementsPrinted = 0
    for i in range(256):
        output = 0
        outputShift = 21

        for j in range(8):
            if( i & (1<<j) > 0 ):
                output |= j << outputShift
                outputShift -= 3
        
        if elementsPrinted == 8:
            print("")
            elementsPrinted = 0

        print('0b{:024b}'.format(output), end=", ")
        elementsPrinted = elementsPrinted + 1
    print("")

def create4xShuffleBitMaskLUT():
    elementsPrinted = 0
    for i in range(16):
        output = 0
        outputShift = 6

        for j in range(4):
            if( i & (1<<j) > 0 ):
                output |= j << outputShift
                outputShift -= 2
        
        if elementsPrinted == 8:
            print("")
            elementsPrinted = 0
            
        print('0b{:08b}'.format(output), end=", ")
        elementsPrinted = elementsPrinted + 1
    print("")
    

print("4x shuffle mask:")
create4xShuffleBitMaskLUT()
print("=================")
print("8x shuffle mask:")
create8xShuffleBitMaskLUT()
print("=================")