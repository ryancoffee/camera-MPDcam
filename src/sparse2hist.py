#!/usr/bin/python3

import numpy as np
import h5py
import sys
import array
from scipy.sparse import coo_matrix

class IncrementalCOOMatrix(object):

    def __init__(self, shape, dtype):

        if dtype is np.int32:
            type_flag = 'i'
        elif dtype is np.int64:
            type_flag = 'l'
        elif dtype is np.float32:
            type_flag = 'f'
        elif dtype is np.float64:
            type_flag = 'd'
        else:
            raise Exception('Dtype not supported.')

        self.dtype = dtype
        self.shape = shape

        self.rows = array.array('i')
        self.cols = array.array('i')
        self.data = array.array(type_flag)

    def append(self, i, j, v):

        m, n = self.shape

        if (i >= m or j >= n):
            raise Exception('Index out of bounds')

        self.rows.append(i)
        self.cols.append(j)
        self.data.append(v)

    def tocoo(self):

        rows = np.frombuffer(self.rows, dtype=np.int32)
        cols = np.frombuffer(self.cols, dtype=np.int32)
        data = np.frombuffer(self.data, dtype=self.dtype)

        return coo_matrix((data, (rows, cols)),shape=self.shape)

    def __len__(self):

        return len(self.data)


def main():
	if len(sys.argv)<2:
		print('syntax: %s <sparse filename(s)>'%sys.argv[0])
		return
	hbins = np.arange(0,256+1,dtype=int)
	h = np.zeros((hbins.shape[0]-1,),dtype=int)
	for fname in sys.argv[1:]:
		data = np.loadtxt(fname,max_rows=10000)
		frame = 0
		nframes = int(data[-1][0]) + 1
		shape = (64,32)
		sparseframes = [IncrementalCOOMatrix(shape, np.int32)]*nframes
		for i in range(data.shape[0]):
			sparseframes[ int(data[i,0]) ].append(int(data[i][1]),int(data[i][2]),int(data[i][3]))
	
		for f in range(len(sparseframes)):
			frame = sparseframes[f].tocoo()
			h += np.histogram(np.sum(frame,axis=1).astype(float)/float(len(sparseframes)) , hbins )[0]
	print(h)
	return

if __name__ == '__main__':
	main()

