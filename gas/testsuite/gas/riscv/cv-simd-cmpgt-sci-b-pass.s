target: 
    #Register Tests
    cv.cmpgt.sci.b x0, x0, 20
    cv.cmpgt.sci.b x1, x1, 20
    cv.cmpgt.sci.b x2, x2, 20
    cv.cmpgt.sci.b x8, x8, 20
    cv.cmpgt.sci.b x20, x20, 20
    cv.cmpgt.sci.b x31, x31, 20
    #Immediate Values Test
    cv.cmpgt.sci.b x6, x7, -32
    cv.cmpgt.sci.b x6, x7, 0
    cv.cmpgt.sci.b x6, x7, 31
