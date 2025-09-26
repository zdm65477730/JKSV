#!/usr/bin/env python3
# coding: utf-8

import zlib
import os
import sys
import shutil

def main():
    # This is run from the makefile. All paths must be relative to it.
    inputDir = "./Text/Files"
    outputDir = "./romfs/Text"

    if(os.path.exists(outputDir)):
        shutil.rmtree(outputDir)

    os.mkdir(outputDir)
    for entry in os.listdir(inputDir):

        inputPath = f"{inputDir}/{entry}";
        inputSize = os.path.getsize(inputPath)
        inputFile = open(file=inputPath, mode="rb")
        if inputFile.closed:
            print(f"Error opening {inputPath} for reading!")
            continue

        inputBuffer = inputFile.read(inputSize)
        outputBuffer = zlib.compress(inputBuffer, 9)

        outputPath = f"{outputDir}/{entry}.z"
        outputFile = open(file=outputPath, mode="wb")
        if outputFile.closed:
            print("Error opening {outputPath} for writing!")
            continue

        inputSizeBytes = inputSize.to_bytes(4, byteorder="little")
        outputSizeBytes = len(outputBuffer).to_bytes(4, byteorder="little")
        outputFile.write(inputSizeBytes)
        outputFile.write(outputSizeBytes)
        outputFile.write(outputBuffer)

    return 0

if __name__ == "__main__":
    sys.exit(main())
