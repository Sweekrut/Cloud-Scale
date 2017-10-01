#@File: myhmm.py
#@Description: Includes methods implementing Markov Chain. 
#              Includes training code as well
#Author: Sweekrut Suhas Joshi

#File imports
import numpy as np
from random import randint
import math
import operator
import os

#Description: Helper method to multiply matrix and row
#
#parameters: 
#   row: row to multiply
#   matrix: Transition table
#
#Return: Returns predicted value
def multiplyMatrix(row, matrix):
    result = [0.0 for i in range(len(row))]
    for i in range(len(row)):
        for j in range(len(row)):
            result[i] += row[j]*matrix[j][i]

    return result

#Description: Predict using transition table and input data
#
#parameters: 
#   data: input data
#   pred_count: How much to prediction
#   CountMatric: Transition table
#   symbols: Symbol count
#   states: State count
#   mmtablename: Transition table file name
#
#Return: Returns predicted value pred_count in the future
def MemPredict(data, pred_count, CountMatrix, symbols, states, mmtablename):
    
    #transition matrix = MxM
    M = symbols/states
    
    #Get current state
    tempstate = (float)(data)/M
    currentstate = int(math.ceil(tempstate))

    prediction = []
    
    #Get current state probability matrix
    l = CountMatrix[currentstate]
    tempList = [0.00 for i in range(len(l))]
    tempList[currentstate] = 1.0
    #predict pred_count steps in the future
    for i in range (0, pred_count):
        tempList = multiplyMatrix(tempList, CountMatrix)
        #tempList = CountMatrix[currentstate]
        state, value = max(enumerate(tempList), key=operator.itemgetter(1))
        prediction.append(((state*M) + ((state+1)*M))/2)
        currentstate = state

    return prediction[pred_count-1]

#Description: Load transition matrix from file and predict
#
#parameters: 
#   dataval: input data
#   pred_count: How much to prediction
#   mmtablename: Transition table file name
#   symbols: Symbol count
#   states: State count
#
#Return: Returns predicted value pred_count in the future
def Predict (dataval, pred_count, mmtablename, symbols = 100, states = 50):
    
    filehandle = open (mmtablename, 'r')
    CountMatrix = [[0.0 for x in range(states+2)] for y in range(states+2)]
    
    #read table from file
    with open(mmtablename, "rt") as f:
        for line in f:
            data = line.split(',')
    data = [float(i) for i in data]

    x = 0
    for i in range (1, len(CountMatrix)):
        for j in range (1, len(CountMatrix)):        
            CountMatrix[i][j] = data[x]
            x += 1
    print x 
    #for i in range(1, len(CountMatrix)):    
        #content = filehandle.readline()
        #data = content.split(',')
        #CountMatrix[i] = [float(j) for j in data]
    
    return MemPredict(dataval, pred_count, CountMatrix, symbols, states, mmtablename)   
    
#Description: Train using input data, write matrix to file and predict
#
#parameters: 
#   data: input data
#   pred_count: How much to prediction
#   mmtablename: Transition table file name
#   symbols: Symbol count
#   states: State count
#
#Return: Returns predicted value pred_count in the future    
def MarkovChain(data, pred_count, mmtablename, symbols = 100, states = 50):
    
    M = symbols/states

    CountMatrix = [[0.0 for x in range(states+1)] for y in range(states+1)]

    countRow = [0 for x in range(len(CountMatrix[0]))]

    #Train using input data
    for i in range(0, len(data)-1):
        index1 = (int)(math.ceil((float)(data[i]/M)))
        index2 = (int)(math.ceil((float)(data[i+1]/M)))
        CountMatrix[index1][index2] += 1
        countRow[index1] += 1

    for i in range(1, len(CountMatrix)):
        for j in range(1, len(CountMatrix)):
            if countRow[i] > 0:
                CountMatrix[i][j] = (float)(CountMatrix[i][j]/countRow[i])
    
    outfile = open (mmtablename, 'w')
    
    #Write table to file
    for i in range(0, len(CountMatrix)-1):
        for j in range(0, len(CountMatrix)):
            outfile.write ("%f," %(CountMatrix[i][j]));
    for i in range(0, len(CountMatrix)-1):
        outfile.write ("%f," %(CountMatrix[len(CountMatrix)-1][i]));

    outfile.write ("%f" %(CountMatrix[len(CountMatrix)-1][len(CountMatrix)-1]));

    return MemPredict(data[len(data)-1], pred_count, CountMatrix, symbols, states, mmtablename)

