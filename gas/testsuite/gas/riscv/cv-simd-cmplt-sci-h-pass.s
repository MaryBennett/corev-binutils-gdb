target:
    #Register Tests
    cv.cmplt.sci.h x0, x0, 20
    cv.cmplt.sci.h x1, x1, 20
    cv.cmplt.sci.h x2, x2, 20
    cv.cmplt.sci.h x8, x8, 20
    cv.cmplt.sci.h x20, x20, 20
    cv.cmplt.sci.h x31, x31, 20
    #Immediate Values Test
    cv.cmplt.sci.h x6, x7, -32
    cv.cmplt.sci.h x6, x7, 0
    cv.cmplt.sci.h x6, x7, 31
