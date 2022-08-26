import subprocess
import time

base = "/pub/scratch/lfalardi/my_bachelor_thesis/covertree/"
xclbin = base+"xclbin/vadd.hw.xclbin"
host = base+"cvtree"
set2 = (1000,1500,2000)
set3 = (32) # Reasonable: between 32 and 1024
tot = len(set2)*10
c = 0
perc = 0
old_perc = 0
for n_points_tree in set2:
    call = "%s %s %i %i %i 0" % (host, xclbin, n_points_tree, 1, 32)
    # print(call)
    # exec(call)
    for _ in range(10):
        p = subprocess.call(call, shell=True, stdout=subprocess.DEVNULL)
        c += 1
        # print(p)
        time.sleep(3)
        perc = round(c/tot*100,1)
        if(perc != old_perc):
            print(perc,'%')
            old_perc = perc
