import subprocess

base = "/pub/scratch/lfalardi/my_bachelor_thesis/covetree_simple/"
xclbin = base+"xclbin/vadd.hw.xclbin"
host = base+"covetree"
set1 =(100,500,1000)
set2 = (600,650,700,750,800,850,827,900,950,1000)
tot = len(set1)*len(set2)
c = 0
perc = 0
old_perc = 0
for n_query_points in set1:
    for n_points_tree in set2:
        call = "%s %s %i %i 0" % (host, xclbin, n_points_tree, n_query_points)
        # print(call)
        # exec(call)
        c += 1
        p = subprocess.call(call, shell=True, stdout=subprocess.DEVNULL)
        if(p>0):
            raise Exception("TEST FAILED "+str(n_points_tree)+" "+str(n_query_points))
        perc = round(c/tot*100,1)
        if(perc != old_perc):
            print(perc)
        old_perc = perc

tot_test = 0
for x in set1:
    tot_test = x*len(set2)
print("Test successful on %i samples" % tot_test)