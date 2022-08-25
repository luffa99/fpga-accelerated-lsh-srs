import subprocess
import time

base = "/pub/scratch/lfalardi/my_bachelor_thesis/covertree/"
xclbin = base+"xclbin/vadd.hw.xclbin"
host = base+"cvtree"
set1 = (1,100,1000,5000,7000)
set2 = (100,500,1000)
set3 = (32,80,160,800,1024) # Reasonable: between 32 and 1024
tot = len(set1)*len(set2)*len(set3)*10
c = 0
perc = 0
old_perc = 0
for n_query_points in set1:
    for n_points_tree in set2:
        for size_of_vectors in set3:
            call = "%s %s %i %i %i 0" % (host, xclbin, n_points_tree, n_query_points, size_of_vectors)
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
