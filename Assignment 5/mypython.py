# Brandon Lee
# Assignment 5 - Python Exploration
# CS 344
# Written in Python 3.4

from random import randint

lowerCaseAlphabet = ['a', 'b', 'c', 'd', 'e', 'f', 'g', 'h', 'i',
                     'j', 'k', 'l', 'm', 'n', 'o', 'p', 'q', 'r',
                     's', 't', 'u', 'v', 'w', 'x', 'y', 'z']

def randomCharacters():
    return lowerCaseAlphabet[randint(0,25)]

def createFiles():
    file1 = open("File1.txt","w+")
    file1Contents = ""
    for i in range(10):
        randomCharacter = randomCharacters()
        file1.write(randomCharacter)
        file1Contents += randomCharacter
    file1.close()

    file2 = open("File2.txt", "w+")
    file2Contents = ""
    for i in range(10):
        randomCharacter = randomCharacters()
        file2.write(randomCharacter)
        file2Contents += randomCharacter
    file2.close()

    file3 = open("File3.txt", "w+")
    file3Contents = ""
    for i in range(10):
        randomCharacter = randomCharacters()
        file3.write(randomCharacter)
        file3Contents += randomCharacter
    file3.close()

    print("File 1 Contents: " + file1Contents)
    print("File 2 Contents: " + file2Contents)
    print("File 3 Contents: " + file3Contents)

def randomProduct():
    x = randint(1, 42)
    y = randint(1, 42)
    product = x * y
    print("The product of {0} and {1} is: {2}".format(str(x), str(y), str(product)))

def main():
    createFiles()
    randomProduct()

main()
