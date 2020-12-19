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

def printimg(img):
	for i in range(img.shape[0]):
		line = ''
		for j in range(img.shape[1]):
			line += '%i '%img[i,j]
		print(line)
	return img

def main():
	if len(sys.argv)<2:
		print('syntax: %s <sparse filename(s)>'%sys.argv[0])
		return
	h = np.zeros((32,256),dtype=int)
	hbins = [i for i in range(257)]
	sumimg = np.zeros((64,32),dtype=int)
	totframes = 0
	for fname in sys.argv[1:]:
		data = np.loadtxt(fname,max_rows=100000)
		print(data[:10,:])
		framenum = -1
		nframes = int(data[-1][0]) + 1
		totframes += nframes
		print(nframes)
		shape = (64,32,nframes)
		#sparseframes = [IncrementalCOOMatrix(shape, np.int32)]*nframes
		denseframes = np.zeros((shape[0],shape[1],nframes),dtype=int)
		for row in data:
			#sparseframes[ int(data[i,0]) ].append(int(data[i][1]),int(data[i][2]),int(data[i][3]))
			denseframes[int(row[1]),int(row[2]),int(row[0])] = int(row[3])
	
		signal = np.sum(denseframes,axis=0)
		#for row in signal:
		for r in range(signal.shape[0]):
			h[r] += np.histogram(signal[r],hbins)[0]
			print(h[r][:10])

	return
'''
		for f in range(denseframes.shape[2]):
			print(f)
			#frame = f.tocoo().toarray()
			#sumimg += frame
				#h[i,np.uint8(signal[i])] += 1
			np.savetxt('%s.%i'%(fname,f),denseframes[:,:,f],fmt='%i')
			f += 1
			#print(signal)
		#printimg(sumimg)
	for i in range(len(h)):
		print(h[i][:10])
	return
'''

if __name__ == '__main__':
	main()

