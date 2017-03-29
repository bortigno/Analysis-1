"""
"""
import ROOT as R
R.gROOT.SetBatch(R.kTRUE)

import categories
from generatingFunctions import *
from Configuration.higgs.Iowa_settings import *
from categories import *
from common import *
import argparse

parser = argparse.ArgumentParser()
parser.add_argument("-n", "--number", type=int, default=0, help="number identifies the function to run")
parser.add_argument('-v', '--verbose', action='store_true', default=False, help='Verbose debugging output')
parser.add_argument('-m', '--mode', type=str, default='Iowa', help='Run in Iowa, UF_AWB, or UF_AMC mode')
args = parser.parse_args()

def generate_backgroundFits():
    for category in run1Categories:
        ws = R.RooWorkspace("higgs")
        aux.buildMassVariable(ws, **diMuonMass125)
        modelsToUse = bersteinsPlusPhysModels
        counter = 0;
        for m in modelsToUse:
            m.color = colors[counter]
            counter+=1
        backgroundFits((category, diMuonMass125), ws, data, modelsToUse,
            pathToDir=backgroundfitsDir,groupName="bersteinsPlusPhysModels")

def generate_signalFitInterpolations():
    for category in run1Categories:
        ws = R.RooWorkspace("higgs")
        aux.buildMassVariable(ws, **diMuonMass125)
        signalFitInterpolation(category, ws, 
            [
                (vbf120, singleGaus120, diMuonMass120),
                (vbf125, singleGaus125, diMuonMass125),
                (vbf130, singleGaus130, diMuonMass130),
            ],
            pathToDir=singalfitinterpolationsDir
        )

def generate_signalFits():
    initialValuesFromTH1 = True
    for category in run1Categories:
        ws = R.RooWorkspace("higgs")
        aux.buildMassVariable(ws, **diMuonMass125)
        for modelToUse in [singleGaus125, doubleGaus125, tripleGaus125]:
            modelToUse.color = R.kRed
            signalFit((category, diMuonMass125), ws, vbf125, modelToUse, pathToDir=signalfitsDir, initialValuesFromTH1=initialValuesFromTH1)
            signalFit((category, diMuonMass125), ws, glu125, modelToUse, pathToDir=signalfitsDir, initialValuesFromTH1=initialValuesFromTH1)
            signalFit((category, diMuonMass125), ws, wm125, modelToUse, pathToDir=signalfitsDir, initialValuesFromTH1=initialValuesFromTH1)
            signalFit((category, diMuonMass125), ws, wp125, modelToUse, pathToDir=signalfitsDir, initialValuesFromTH1=initialValuesFromTH1)
            signalFit((category, diMuonMass125), ws, zh125, modelToUse, pathToDir=signalfitsDir, initialValuesFromTH1=initialValuesFromTH1)
        aux.buildMassVariable(ws, **diMuonMass120)
        for modelToUse in [singleGaus120, doubleGaus120, tripleGaus120]:
            modelToUse.color = R.kRed
            signalFit((category, diMuonMass120), ws, vbf120, modelToUse, pathToDir=signalfitsDir, initialValuesFromTH1=initialValuesFromTH1)
            signalFit((category, diMuonMass120), ws, glu120, modelToUse, pathToDir=signalfitsDir, initialValuesFromTH1=initialValuesFromTH1)
            signalFit((category, diMuonMass120), ws, wm120, modelToUse, pathToDir=signalfitsDir, initialValuesFromTH1=initialValuesFromTH1)
            signalFit((category, diMuonMass120), ws, wp120, modelToUse, pathToDir=signalfitsDir, initialValuesFromTH1=initialValuesFromTH1)
            signalFit((category, diMuonMass120), ws, zh120, modelToUse, pathToDir=signalfitsDir, initialValuesFromTH1=initialValuesFromTH1)
        aux.buildMassVariable(ws, **diMuonMass130)
        for modelToUse in [singleGaus130, doubleGaus130, tripleGaus130]:
            modelToUse.color = R.kRed
            signalFit((category, diMuonMass130), ws, vbf130, modelToUse, pathToDir=signalfitsDir, initialValuesFromTH1=initialValuesFromTH1)
            signalFit((category, diMuonMass130), ws, glu130, modelToUse, pathToDir=signalfitsDir, initialValuesFromTH1=initialValuesFromTH1)
            signalFit((category, diMuonMass130), ws, wm130, modelToUse, pathToDir=signalfitsDir, initialValuesFromTH1=initialValuesFromTH1)
            signalFit((category, diMuonMass130), ws, wp130, modelToUse, pathToDir=signalfitsDir, initialValuesFromTH1=initialValuesFromTH1)
            signalFit((category, diMuonMass130), ws, zh130, modelToUse, pathToDir=signalfitsDir, initialValuesFromTH1=initialValuesFromTH1)

def generate_distributions():
    logY = True
    for category in run1Categories:
        for vname in varNames:
            variable = {}
            variable["name"]=vname
            variable["min"]=-0.999
            variable["max"]=-0.999
            if category!="NoCats" and vname=="DiMuonMass":
                variable["min"] = 110
                variable["max"] = 160
            distributions((category, variable), data, 
                [glu125, vbf125, wm125, wp125, zh125],
                [wJetsToLNu, wwTo2L2Nu, wzTo3LNu, tt, dy], pathToDir=distributionsDir,
                logY=logY)

if __name__=="__main__":
    if args.number == 0:
        generate_distributions()
    elif args.number == 1:
        generate_signalFits()
    elif args.number == 2:
        generate_backgroundFits()
    elif args.number == 3:
        generate_signalFitInterpolations()