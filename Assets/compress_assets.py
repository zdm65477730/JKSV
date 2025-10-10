#!/usr/bin/env python3
# coding: utf-8

import zlib
import os
import sys
import shutil

def ensure_fresh_dir(path: str):

    if os.path.exists(path):
        shutil.rmtree(path)

    os.mkdir(path)


def compress_to_romfs(input: str, output: str):

    print(f"Compressing {input} -> {output}")
    inputSize: int =os.path.getsize(input)
    with open(file=input, mode="rb") as inputFile:

        inputBuffer: bytes = inputFile.read()
        outputBuffer: bytes = zlib.compress(inputBuffer, 9)

        with open(file=output, mode="wb") as outputFile:

            inputSizeBytes: bytes = inputSize.to_bytes(length=4, byteorder="little")
            outputSizeBytes: bytes = len(outputBuffer).to_bytes(length=4, byteorder="little")

            outputFile.write(inputSizeBytes)
            outputFile.write(outputSizeBytes)
            outputFile.write(outputBuffer)



def main() -> int:

    # This is run from the makefile. All paths must be relative to it, not the script.
    textInput:  str  = "./Assets/Text"
    textOutput: str  = "./romfs/Text"

    ensure_fresh_dir(textOutput)

    for entry in os.listdir(textInput):

        inputPath: str    = f"{textInput}/{entry}"
        outputPath: str   = f"{textOutput}/{entry}.z"

        compress_to_romfs(inputPath, outputPath)

    return 0

if __name__ == "__main__":
    sys.exit(main())
