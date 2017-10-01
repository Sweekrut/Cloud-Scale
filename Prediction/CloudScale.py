#!/usr/bin/python
#@File: CloudScale.py
#@Description: Implements Cloud Scale prediction algorithms
#Author: Sweekrut Suhas Joshi

#File imports
from scipy.fftpack import fft,ifft,fftfreq
from scipy.stats.stats import pearsonr
import numpy as np
from random import randint
import operator
import myhmm
import math
import sys
import os.path
from fastdtw import fastdtw
from scipy.spatial.distance import euclidean

import matplotlib.pyplot as plt

#Description: Reads data from file
#
#parameters: 
#   fname: File name to read from
#   count: Number of values to read
#
#Return: Returns read data list
def ReadFromFile(fname, count):
    with open(fname, "rt") as f:
        for line in f:
            data = line.split(',')    
    data = [float(i) for i in data[0:len(data)-1]]

    #Get last count values
    if len(data) > count:
        data = data[-int(count):]
    return data

#Description: Gets padding value for given input
#
#parameters: 
#   data: Input data for which padding needs to be obtained
#
#Return: Returns padding value
def GetPaddingValue(data):    
    #Perform fft
    y = fft(data)
    
    k = int(len(y))
    topksize = int(((k*80))/100)
    
    burst_fft = y[-topksize:]
    
    max_values = [0] * len(y)
    offset = k - topksize
    for i in range(offset,k):
        max_values[i] = y[i]

    #inverse fft on top 80 frequency
    burst_pattern = ifft(max_values)

    #Padding is either max of burst_pattern or 80% value.
    positive_value_count = sum(1 for x in burst_pattern if x > 0)
    for i in range(len(burst_pattern)):
        if burst_pattern[i] < 0:
            burst_pattern[i] = 0
    
    if positive_value_count >= (len(burst_pattern)/2):
        return abs(max(burst_pattern))
    else:
        return abs(np.percentile(burst_pattern, 80))

#Description: Check if arrays satisfy pearson corelation
#
#parameters: 
#   PatternArrays: List of patterns which need to be compared
#   Threshold: Pearson corelation threshold
#
#Return: Returns true if all pairs of lists satisfy pearson threshold. Else false
def ConfirmPearsonCorelation (PatternArrays, Threshold):
    #check if all windows match
    W = len (PatternArrays)
    for i in range(W):
        for j in range(W):
            if len(PatternArrays[i]) == len(PatternArrays[j]) and i != j:
                r, p = pearsonr(PatternArrays[i], PatternArrays[j])
                if abs(r) < Threshold: 
                    #print "pearson failed"
                    return False
    return True

#Description: Finds the signature given a set of pattern lists
#
#parameters: 
#   PatternArrays: List of patterns which need to be compared
#
#Return: Returns the signature array.
def CalculateAvgValues (PatternArrays):
    W = len (PatternArrays)
    sum_val = 0
    avg = []

    for i in range(len(PatternArrays[0])):
        sum_val = 0
        for j in range(W):
            sum_val += PatternArrays[j][i]
        avg.append(sum_val/W);    
    return avg

#Description: Use fastDTW lib to find offset in lists
#
#parameters: 
#   testSet: Input of size = pattern size
#   pattern: Pattern obtained from fft
#
#Return: Returns time offset
#Author: Suhaskrishna Gopalkrishna
def fastDTW (testSet, pattern):
    if len(testSet) != len(pattern):
        return 0

    referenceSet = np.concatenate([pattern,pattern])
    distance, path = fastdtw(testSet, referenceSet, dist=euclidean)
    for i in range(len(path)):
        if path[i][0] == 1:
            return ((path[i][1] + len(pattern) - 2)%len(pattern))

#Description: Self implementation of fast DTW
#
#parameters: 
#   predicted: Pattern obtained from fft
#   real: Input of size = pattern size
#
#Return: Returns time offset     
def DTW (predicted, real):
    if len(predicted) != len(real):
        return 0

    N = len(predicted)

    isincreasing = True

    # Check if pattern is increasing or decreasing
    if predicted[0] > predicted[1]:
        isincreasing = False

    #first find the index with least deviation
    minerr = 1000
    index = 0
    final = 0
    start = 0
    err = 1000
    got = False

    while True:
        if start >= N:
            return final

        for i in range(start, N):
            if abs(predicted[i]- real[N-1]) < minerr:
                minerr = abs(predicted[i]-real[N-1])
                index = i
                got = True

        if got == False:
            return final

        #Now check if pattern matched one ahead
        if isincreasing == False:
            if real[index] > real[(index+1)%N]:
                error = abs(real[(index+1)%N] - predicted[(index+1)%N])
                if error < err:
                    err = error
                    final = index
        else:
            if real[index] < real[(index+1)%N]:
                error = abs(real[(index+1)%N] - predicted[(index+1)%N])
                if error < err:
                    err = error
                    final = index

        start = index+1
        got = False

#Description: Predicts using FFT
#
#parameters: 
#   data_fft: Input data for fft
#   inputfile: Input file name
#   outputfile: File name to write output in 
#   sampling_interval: sampling rate in seconds
#   islongterm: need to predict long term or next value?
#   name: VM name to write to out file
#
#Return: Returns 1 if fft was successful. Else 0   
def PredictUsingFFT (data_fft, inputfile, outputfile, sampling_interval, islongterm, name):

    PEARSON_THRESHOLD = (float)(0.85)

    #Calculate fft
    y = abs(fft(data_fft))
    y = y[0:(len(y)/2 + 1)]
    
    #set first value as 0 as we dont want to consider this amplitude value
    y[0] = 0.00

    # sample spacing
    T = 1
    # Number of samplepoints
    N = len(data_fft)

    xf = np.linspace(0.0, 1.0/(2.0*T), (N/2) + 1)

    #find max val and its index
    index, value = max(enumerate(y), key=operator.itemgetter(1))
        
    #Get the lowest dominating frequency
    fd = xf[index]

    if fd == 0:
        return 0

    #find pattern window size Q
    Z = int(math.ceil(1/fd))
    do_hmm = False

    if Z < 3:
        return 0;

    #If fft says that the whole length of input is repeating, then perform mm
    if Z == N:
        print "Pattern size is equal to Data size"
        do_hmm = True    

    Q = int(N/Z)

    #Ignore last parrern subarray if length of subarray does not split the input equally
    if len(data_fft)%Z != 0:
        data_new = data_fft[0:len(data_fft) - len(data_fft)%Z]
        P = np.array_split(data_new, Q)
    else:
        P = np.array_split(data_fft, Q)
    
    #check if all windows match
    pearson = ConfirmPearsonCorelation (P, PEARSON_THRESHOLD)
    prediction = []
    if do_hmm == False and pearson == True:
        
        if islongterm == False:
            print name," Pattern repeates with period %d" %Z
        
        avg = CalculateAvgValues (P)
        #Perform DTW
        #Get 100 most recent values
        latestdata = ReadFromFile(inputfile, Z)
        offset_index = fastDTW (avg, latestdata)        
        outfile = open (outputfile, 'a')
        if islongterm == False:
            outfile.write ("%s\n" %(avg[(offset_index+1)%Z]))
        else:
            #1 in in the future
            loopcount = int(100/sampling_interval)
            outfile.write ("%s\n" %(avg[(offset_index+loopcount)%Z]))
          
        return 1
    else:
        return 0

#Description: Trains and Predicts using MM
#
#parameters: 
#   inputfile: Input file name
#   outputfile: File name to write output in 
#   mmname: File where transition table needs to be written.
#   predcount: How much to predict
#
#Return: Returns true if successful, else false        
def TrainAndPredictUsingMM (inputfile, outputfile, mmname, predcount):    
    #print ("Have to train")
    #read hmm inputs
    outfile = open (outputfile, 'a')
    
    data = ReadFromFile (inputfile, 2000)
    if len(data) < 2000:
        print "TrainMM"
        outfile.write ("999\n999\n999")
        return False
        
    data_hmm = []
    for i in range(len(data)):
        if data[i] <= 100:
            data_hmm.append(data[i])

    prediction = myhmm.MarkovChain (data_hmm, predcount, mmname, 100, 50)
     
    outfile.write ("%s\n" %(prediction))
    
    return True

#Description: Predicts using MM. Asumes transition table already exists
#
#parameters: 
#   inputfile: Input file name
#   outputfile: File name to write output in 
#   mmname: File where transition table needs to be written.
#   predcount: How much to predict
#
#Return: None
def PredictMM(inputfile, outputfile, mmfile, predcount):
    #read hmm inputs
    datalist = ReadFromFile (inputfile, 1)
    data = datalist[0]
            
    if data > 100 or data < 0:
        print "Invalid input: ", data
        return
        
    prediction = myhmm.Predict (data, predcount, mmfile, 100, 50)

    outfile = open(outputfile, 'a')

    outfile.write ("%s\n" %(prediction))   

#Description: Predicts using MM and finds out if we need to train or not
#
#parameters: 
#   inputfile: Input file name
#   outputfile: File name to write output in 
#   mmfile: File where transition table needs to be written.
#   sampling_interval: samplring rate
#   islong: Long term or short term prediciton?
#   name: VM name
#
#Return: Returns true if successful. Else false.
    
def UseMM (inputfile, outputfile, mmfile, sampling_interval, islong, name):

    count = 1
    if islong == True:
        count = int(100/sampling_interval)
    else:
        print name, " Predicting using Markov model" 
 
    if os.path.exists(mmfile):
        PredictMM(inputfile, outputfile, mmfile, count)
        return True

    return TrainAndPredictUsingMM (inputfile, outputfile, mmfile, count)

#Description: Start of execution
#
#parameters: 
#   inputfile: Input file name
#   outputfile: File name to write output in 
#   mmfile: File where transition table needs to be written.
#   sampling_interval: samplring rate
#   name: VM name
#
#Return: None

def main(inputfile, outputfile, mmfile, sampling_interval, name):
   
    interval = int(sampling_interval)
    #Predict for capping
    data_fft = ReadFromFile(inputfile, 200)
    outfile = open (outputfile, 'w')
    outfile.close()
    #Enough data to do fft
    if len(data_fft) < 200:        
        outfile = open (outputfile, 'a')
        outfile.write ("999\n999\n999")   
        return 2

    #Else try fft
    retval = PredictUsingFFT (data_fft, inputfile, outputfile, interval, False, name)
    
    #If fft failed use MM
    if retval == 0:
        if UseMM (inputfile, outputfile, mmfile, interval, False, name) == False:
            return 2

    #predict for long term
    longdata = ReadFromFile(inputfile, 2000)
    if len(longdata) < 1999:
        print "< 1999"
        
        outfile = open (outputfile, 'a')
        outfile.write ("999\n")            
    else:     
        retval = PredictUsingFFT (data_fft, inputfile, outputfile, interval, True, name)
        
        if retval == 0:
            UseMM (inputfile, outputfile, mmfile, interval, True, name)
        
    #Get padding value
    burst_array = ReadFromFile (inputfile, 100)
    paddingval = int(GetPaddingValue(burst_array))
    outfile = open (outputfile, 'a')
    outfile.write ("%s\n" %(paddingval))
    outfile.close()

    #print "-----------DONE-------------"

#MAIN FUNCTION
if __name__ == '__main__':
    main(sys.argv[1], sys.argv[2], sys.argv[3], sys.argv[4], sys.argv[5])

