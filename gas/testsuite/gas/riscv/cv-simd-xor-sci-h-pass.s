target:
    #Register Tests
    cv.xor.sci.h x0, x0, 20
    cv.xor.sci.h x1, x1, 20
    cv.xor.sci.h x2, x2, 20
    cv.xor.sci.h x8, x8, 20
    cv.xor.sci.h x20, x20, 20
    cv.xor.sci.h x31, x31, 20
    #Immediate Values Test
    cv.xor.sci.h x6, x7, -32
    cv.xor.sci.h x6, x7, 0
    cv.xor.sci.h x6, x7, 31
