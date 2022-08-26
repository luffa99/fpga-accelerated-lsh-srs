import subprocess
import time
import numpy as np

base = "/pub/scratch/lfalardi/my_bachelor_thesis/covertree/"
xclbin = base+"xclbin/vadd.hw.xclbin"
host = base+"cvtree"
set1 = (1,128,512,1024,4096,8192)
set2 = (100,500,1000,1500,2000)
set3 = (32,128,512,1024) # Reasonable: between 32 and 1024
tot = len(set1)*len(set2)*len(set3)*3
c = 0
perc = 0
old_perc = 0

tot_points = 0
st = time.time()
tot_points_tot = np.sum([i*len(set2)*len(set3)*3 for i in set1])
for n_query_points in set1:
    for n_points_tree in set2:
        for size_of_vectors in set3:
            call = "%s %s %i %i %i 0" % (host, xclbin, n_points_tree, n_query_points, size_of_vectors)
            # print(call)
            # exec(call)
            for _ in range(3):
                p = subprocess.call(call, shell=True, stdout=subprocess.DEVNULL)
                c += 1
                tot_points += n_query_points
                perc = round(c/tot*100,1)
                if(perc != old_perc):
                    print(perc,'%')
                    old_perc = perc
        et = time.time()
        elapsed_time = et - st
        time_per_point = elapsed_time/tot_points
        estimation = time_per_point*(tot_points_tot-tot_points)
        estimation = estimation + 0.1*estimation
        print("Remaining time estimation: "+str(estimation/60)+" min ("+str((estimation/60)/60)+ " h)")

                
