#!/usr/bin/env python2
#
# File: test_covertree.py
# Date of creation: 11/20/08
# Copyright (c) 2007, Thomas Kollar <tkollar@csail.mit.edu>
# Copyright (c) 2011, Nil Geisweiller <ngeiswei@gmail.com>
# All rights reserved.
#
# This is a tester for the cover tree nearest neighbor algorithm.  For
# more information please refer to the technical report entitled "Fast
# Nearest Neighbors" by Thomas Kollar or to "Cover Trees for Nearest
# Neighbor" by John Langford, Sham Kakade and Alina Beygelzimer
#  
# If you use this code in your research, kindly refer to the technical
# report.

from covertree import CoverTree
from naiveknn import knn
# from pylab import plot, show
from numpy import subtract, dot, sqrt, linalg
import numpy as np
from random import random, seed
import time
# import cPickle as pickle
import pickle
import sys
import csv
import datetime


def distance(p, q):
    # print "distance"
    # print "p =", p
    # print "q =", q
    x = subtract(p, q)
    return sqrt(dot(x, x))
    # return linalg.norm(subtract(p,q))

def test_covertree():
    seed(1)

    total_tests = 0
    passed_tests = 0

    if(len(sys.argv) < 3):
        print ("Usage: script number_of_points max_number_of_children")
        exit(1)
    
    n_points = int(sys.argv[1])
    maxchildren = int(sys.argv[2])
    want_output = int(sys.argv[3])

    # k = 1
    
    pts = [(random(),random(),random(),random(),random(),random()) for _ in range(n_points)]
    #Import points from file:
    # pts = np.genfromtxt("pts.txt",delimiter=",",dtype='float32')
    # unique, counts = np.unique(pts, return_counts=True)

    # result = np.column_stack((unique, counts)) 
    # print (counts)


    gt = time.time

    print ("Build cover tree of", n_points, "6D-points")
    
    t = gt()
    ct = CoverTree(distance, maxchildren)
    for p in pts:
        ct.insert(p)
    b_t = gt() - t
    # print "Building time:", b_t, "seconds"

    # print ("==== Check that all cover tree invariants are satisfied ====")
    # if ct.check_invariants():
    #     print ("OK!")
    #     passed_tests += 1
    # else:
    #     print ("NOT OK!")
    # total_tests += 1
    
    # print "==== Write test1.dot, dotty file of the built tree ===="
    with open("test1.dot", "w") as testDottyFile1:
        ct.writeDotty(testDottyFile1)

    #Save built tree
    level = ct.maxlevel
    visited = [] # List for visited nodes.
    queue = []     #Initialize a queue
    points = {}

    def bfs(visited, node): #function for BFS
        visited.append(node)
        queue.append(node)

        while queue:          # Creating loop to visit each node
            m = queue.pop(0) 
            #print m, end = " "
            childs = {}
            for level in m.children:
                for c in m.getChildren(level):
                    childs[c] = level

            childs_ids = []
            for child in childs:
                if child!=m:
                    childs_ids.append((childs[child],child.id)) #level of child, child_id
            childs_ids.sort(key=lambda tup: -tup[0])  # sorts in place according to level
            points[m.id] = [m.data, childs_ids]

            for neighbour in childs:
                if neighbour not in visited:
                    # print "Node ",m.id, " -> ",neighbour.id,childs[neighbour]
                    visited.append(neighbour)
                    queue.append(neighbour)

    # Driver Code
    # print ("Building graph to export")
    bfs(visited, ct.root)    # function calling
    points = sorted(points.items(), key=lambda x: int(x[0]))
    points = list(map(lambda i : i[1], points))
    for node in points:
        if len(node[1]) < maxchildren: ## max 16 children !!!
            extension = [(69,-1) for el in range(maxchildren-len(node[1]))] ## 69 needs to be > max value!!
            node[1].extend(extension)


    #print(points)
    # for i, p in enumerate(points):
    #     print("Node "+str(i)+" D: "+str(distance(query,p[0])))
    # import json
    # with open('generated_tree_'+str(n_points)+'.json', 'w') as f:
    #     json.dump(points, f)
    with open('generated_tree_'+str(n_points)+'.txt', 'w') as x:
        x.write(str(ct.ignored)+'\n')
        x.write(str(ct.maxlevel)+'\n')
        x.write(str(ct.minlevel)+'\n')
        for node in points:
            for c in node[0]:
                x.write(str(c)+'\n')
            for (a,b) in node[1]:
                x.write(str(a)+'\n')
                x.write(str(b)+'\n')
    
    k = 1

    # This list will be used by naive knn
    points_fn = [x for (x,_) in points]
    

    # print "==== Test saving/loading (via pickle)"
    ctFileName = "test.ct"
    # print "Save cover tree under", ctFileName
    t = gt()
    ct_file = open("test.ct", "wb")
    pickle.dump(ct, ct_file)
    ct_file.close()
    # print "Saving time:", gt() - t
    del ct_file
    del ct
    # load ct
    # print "Reload", ctFileName
    t = gt()
    ct_file = open("test.ct", "rb")
    ct = pickle.load(ct_file)
    ct_file.close()
    # print "Loading time:", gt() - t
    
    print ("==== Test (PYTHON) " + str(k) + "-nearest neighbors cover tree query ====")
    #query = (0.5,0.5,0.5)

    

    with open('querys.txt', newline='') as f:
        rows = list(csv.reader(f))      # Read all rows into a list

    querys = []
    for row in rows:    # Skip the header row and convert first values to integers
        querys.append(float(row[0]))
    
    n_querys = len(querys) // 6;

    # print(querys,n_querys)

    # query = (float(sys.argv[3]),float(sys.argv[4]),float(sys.argv[5]),float(sys.argv[6]),float(sys.argv[7]),float(sys.argv[8]))
    print ("Querys: ",n_querys)

    tot_results = []
    tot_results_2 = []
    tot_results_3 = []
    tot_results_4 = []
    g_time = 0
    for x in range(0,n_querys):

        query = querys[x*6:x*6+6]

        if(want_output):
            print ("Query #",x)
            print (query)

        # t = gt()
        start_time = datetime.datetime.now()    
        #cover-tree nearest neighbor
        results = ct.knn(k, query, True)
        # print "result =", result
        end_time = datetime.datetime.now()
        time_diff = (end_time - start_time)
        execution_time = time_diff.total_seconds() * 1000
        g_time += execution_time
        # print(g_time)
        # ct_t = gt() - t
        # g_time += ct_t
        # print "Time to run a cover tree " + str(k) + "-nn query:", ct_t, "seconds"
        
        ##TESTING RESULTS
        
        # naive nearest neighbor
        t = gt()
        naive_results = knn(4, query, points_fn, distance)
        # print "resultNN =", resultNN
        n_t = gt() - t
        # print "Time to run a naive " + str(k) + "-nn query:", n_t, "seconds"
        # print(results)
        # print(naive_results)

        if all([distance(r, nr) != 0 for r, nr in zip(results, naive_results)]):
            print ("NOT OK!")
            print (results)
            print ("!=")
            print (naive_results)
        else:
            if(want_output):
                print ("OK!")
                # print (results)
            # print "=="
            # print naive_results
            # print "Cover tree query is", n_t/ct_t, "faster"
            passed_tests += 1
        total_tests += 1
        tot_results.append(results[0])
        tot_results_2.append(naive_results[1])
        tot_results_3.append(naive_results[2])
        tot_results_4.append(naive_results[3])

    tot_results.append([passed_tests])

    with open('results.txt', 'w') as x:
        for r in tot_results:
            for a in r:
                x.write(str(a)+'\n')

    with open('results_2.txt', 'w') as x:
        for r in tot_results_2:
            for a in r:
                x.write(str(a)+'\n')
    
    with open('results_3.txt', 'w') as x:
        for r in tot_results_3:
            for a in r:
                x.write(str(a)+'\n')

    with open('results_4.txt', 'w') as x:
        for r in tot_results_4:
            for a in r:
                x.write(str(a)+'\n')

    with open('time.txt', 'w') as x:
        x.write(str(g_time)) # in ms

    # print ("==== Check that all cover tree invariants are satisfied ====")
    # if ct.check_invariants():
    #     print ("OK!")
    #     passed_tests += 1
    # else:
    #     print ("NOT OK!")


    return
    # you need pylab for that
    # plot(pts[0], pts[1], 'rx')
    # plot([query[0]], [query[1]], 'go')
    # plot([naive_results[0][0]], [naive_results[0][1]], 'y^')
    # plot([results[0][0]], [results[0][1]], 'mo')

    # test knn_insert
    print ("==== Test knn_insert method ====")
    t = gt()
    results2 = ct.knn_insert(k, query, True)
    ct_t = gt() - t
    print ("Time to run a cover tree " + str(k) + "-nn query:", ct_t, "seconds")
    
    if all([distance(r, nr) != 0 for r, nr in zip(results2, naive_results)]):
        print ("NOT OK!")
        print (results2)
        print ("!=")
        print (naive_results)
    else:
        print ("OK!")
        print (results2)
        print ("==")
        print (naive_results)
        print ("Cover tree query is", n_t/ct_t, "faster")
        passed_tests += 1
    total_tests += 1

    print ("==== Check that all cover tree invariants are satisfied ====")
    if ct.check_invariants():
        print ("OK!")
        passed_tests += 1
    else:
        print ("NOT OK!")
    total_tests += 1

    print ("==== Write test2.dot, dotty file of the built tree after knn_insert ====")
    with open("test2.dot", "w") as testDottyFile2:
        ct.writeDotty(testDottyFile2)
        
    # test find
    print ("==== Test cover tree find method ====")
    if ct.find(query):
        print ("OK!")
        passed_tests += 1
    else:
        print ("NOT OK!")
    total_tests += 1

    # printDotty prints the tree that was generated in dotty format,
    # for more info on the format, see http://graphviz.org/
    # ct.printDotty()

    # show()

    print (passed_tests, "tests out of", total_tests, "have passed")
    

if __name__ == '__main__':
    test_covertree()
