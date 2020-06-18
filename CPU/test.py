import math
from math import abs,sqrt,sin,cos,atan

from __future__ import print_function
import sys

def eprint(*args, **kwargs):
    print(*args, file=sys.stderr, **kwargs)

def xyz2ell(x_638, y_638, z_638):
    a_638 = 6378137.0000
    b_638 = 6356752.3141
    t_638 = (a_638**2 - b_638**2)/ a_638**2
    lat_638 = math.atan(y_638 / z_638)
    N0_638 = a_638
    h0_638 = math.sqrt(x_638**2 + y_638**2 + z_638**2) - math.sqrt(a_638 * b_638)
    p_638 = math.sqrt(x_638**2 + y_638**2)
    lon0_638 = math.atan(z_638 / (1 - t_638 * (N0_638 / N0_638 + h0_638)) * p_638)
    
    err_N = 1
    err_lon = 1
    err_h = 1
    tolerance_N = 10**(-4)
    tolerance_lon = 10**(-8)
    tolerance_h = 10**(-4)

    N = [N0_638]
    lon_638 = [lon0_638]
    h_638 = [h0_638]
    eprint(h_638)
    eprint("sdfbsdfkkfsd")
    i = 1
    while(abs(err_N) > tolerance_N or abs(err_lon) > tolerance_lon or abs(err_h) > tolerance_h):
        N.append(0)
        lon_638.append(0)
        h_638.append(0)
        eprint(h_638)
        
        N[i] = a_638/sqrt(1-t_638*(sin(lon_638[i-1])**2)
        h_638[i] = p_638/cos(lon_638[i-1]) - N[i]
        lon_638[i] = atan(z_638/(1 - (t_638)* (N[i]/(N[i]+h_638[i]))))
        print(N[i],h[i],lon_638[i])
        err_N = N[i]-N[i-1]
        err_h = h_638[i]-h_638[i-1]
        err_lon = lon_638[i]-lon_638[i-1]
        i += 1

    print((latitude, longitude, ellHeight))
    return(latitude, longitude, ellHeight)


xyz2ell(4216513.9443, 2337251.8449, 4162751.2301):