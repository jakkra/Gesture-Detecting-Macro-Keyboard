import numpy as np

def moveToCenter(img):
    minRow = 100
    maxRow = -1
    minCol = 100
    maxCol = -1
    for rowcol, val in np.ndenumerate(img):
        col = rowcol[0]
        row = rowcol[1]
        if (val > 0):
            if (row < minRow):
                minRow = row
            if (row > maxRow):
                maxRow = row
            
            if (col < minCol):
                minCol = col
            if (col > maxCol):
                maxCol = col

    diffRow = (maxRow - minRow) // 2
    diffCol = (maxCol - minCol) // 2
    centeredImg = np.zeros((28, 28))
    centeredImg[minCol, minRow] = 1
    centeredImg[maxCol, maxRow] = 1
    centeredImg[diffCol + minCol, diffRow + minRow] = 1

    rowOffset = 14 - (diffRow + minRow)
    colOffset = 14 - (diffCol + minCol)

    movedImg = np.zeros((28, 28))

    for rowcol, val in np.ndenumerate(img):
        y = rowcol[0]
        x = rowcol[1]
        if (img[y, x] > 0):
            movedImg[min(max(y + colOffset, 0), 27), min(max(x + rowOffset, 0), 27)] = 1
    
    return movedImg
