#!/usr/bin/python

#Author: Bharath Banglaore Veeranna
#Description:
#Implementation of other prediction algorithm such as mean, Max, Histogram,
#Auto-correlation and auto-regression

import sys
import numpy
from statsmodels.tsa.ar_model import AR

def autocorr(x, t=1):
    return numpy.corrcoef(numpy.array([x[0:len(x)-t], x[t:len(x)]]))

def main():

    fileName = open(sys.argv[1], "r")

    outFile = open(sys.argv[2], "w")

    count = 0
    for line in fileName:
        count = count+1;
        tempList = line.split(',')    

    if count == 0:
        return

    if len(tempList) < 10:
        outFile.write(str(999) + "\n")
        outFile.write(str(999)+ "\n")
        outFile.write(str(999)+ "\n")
        outFile.write(str(999)+ "\n")
        outFile.write(str(999)+ "\n")
        return


    try:
        intList = [float(i) for i in tempList[-11:len(tempList)-1] ]
    except:
        print "Failed"

    
    meanP = 0
    for i in range(0,len(intList)):
        meanP = meanP + intList[i]

    meanP = meanP / len(intList)

    maxP = max(intList)
        
    if len(tempList) > 50:
        startLen = len(tempList)

        if startLen > 101:
            startLen = 101

        intList = [float(i) for i in tempList[len(tempList)-startLen:len(tempList)-1] ]

        histY, histX = numpy.histogram(intList, bins=25, range=(0,100))

        histY = histY.tolist()

        histV = histY.index(max(histY))

        histX = histX.tolist()

        histP = (histX[histV] + histX[histV+1])/2

        # Auto regression:

        model = AR(intList[1:len(intList)-1])

        model_fit = model.fit()
        window = model_fit.k_ar
        coef = model_fit.params

        regP = coef[0]

        values = intList[len(intList)-window:]

        for i in range(window-1):
            regP += coef[i+1] * intList[window - i -1]

        corrValid = False

        for i in range(len(intList)/2):
            corrP = autocorr(intList, i+1)
            t = corrP[1][0]
            if( t > 0.9):
                corrValid = True
                period = i+1
                break;

        if corrValid == True:
            pattern = intList[0:period]

            error = 1000
            pos = 0

            lastValue = intList[len(intList)-1]

            for i in range(period):
                if(abs(lastValue - pattern[i]) < error):
                    error = abs(lastValue - pattern[i])
                    pos = i; 

            autoCorrP = pattern[(pos+1)%period]

        else:
            autoCorrP = meanP

    else:
        histP = 999
        regP = 999
        autoCorrP = 999

    fileName.close()

    meanP = "%.5f" %meanP
    maxP = "%.5f" %maxP
    histP = "%.5f" %histP
    regP = "%.5f" %regP
    autoCorrP = "%.5f" %autoCorrP

    outFile.write(str(meanP)+"\n")
    outFile.write(str(maxP)+ "\n")
    outFile.write(str(histP)+ "\n")
    outFile.write(str(regP)+ "\n")
    outFile.write(str(autoCorrP) + "\n")
    outFile.write("1\n")
    outFile.close()

if __name__ == "__main__":
    main()
