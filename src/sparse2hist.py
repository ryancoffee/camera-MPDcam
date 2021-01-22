#!/usr/bin/python3

import numpy as np
import h5py
import sys
import array
from scipy.sparse import coo_matrix

def printimg(img):
	for i in range(img.shape[0]):
		line = ''
		for j in range(img.shape[1]):
			line += '%i '%img[i,j]
		print(line)
	return img

def main():
	singleline = False
	if len(sys.argv)<2:
		print('syntax: %s <sparse filename(s)>'%sys.argv[0])
		return
	h = np.zeros((32,),dtype=int)
	s = np.zeros((32,64),dtype=int)
	hbins = [i for i in range(33)]
	sumimg = np.zeros((64,32),dtype=int)
	totframes = 0
	printshape = True
	framenum = -1 
	for fname in sys.argv[1:]:
		denseframes = []
		data = np.loadtxt(fname) #,max_rows=1000000)
		print(data.shape)
		if len(data.shape)==1:
			singleline = True
		if singleline:
			print('singleline')
			if (framenum != int(data[b*4]):
				framenum = int(data[b*4]
				totframes += 1
				denseframes += [np.zeros((64,32),dtype=int]
			for b in range(len(data)//4):
				denseframes[-1][int(data[b*4+1]),int(data[b*4+2])] = int(data[b*4+3])
		else:
			print('ascii matrix')
			nframes = int(data[-1][0]) + 1 
			totframes += nframes
			shape = (64,32,nframes)
			denseframes = np.zeros((shape[0],shape[1],nframes),dtype=int)
			for row in data:
				#sparseframes[ int(data[i,0]) ].append(int(data[i][1]),int(data[i][2]),int(data[i][3]))
				denseframes[int(row[1]),int(row[2]),int(row[0])] = int(row[3])
			#signal = np.sum(denseframes,axis=0)
		sumimg += np.sum(denseframes,axis=2)
		for frame in range(shape[-1]):
			for row in range(shape[1]):
				coldata = denseframes[:,row,frame]
				if printshape:
					print('len(row) = %i'%int(len(denseframes[:,row,frame])))
					print(coldata.shape)
					printshape = False
				i = 0
				while i < (len(coldata)-2):
					if coldata[i]*coldata[i+1]*coldata[i+2]:
						h[row] += 1
						i+= 2
					i+=1
				i=0
				while i < len(coldata)-1:
					active = bool(coldata[i])
					if (active):
						j = i + 1
						while (j<len(coldata)-1 and active):
							active *= bool(coldata[j])
							j += 1
						s[row,int(j-i)] += 1
						i = j
					i += 1

					
		'''
		#for row in signal:
		for r in range(signal.shape[0]):
			h[r] += np.histogram(signal[r],hbins)[0]
			print(h[r][:10])
		'''

	headstring = 'total nframes = %i'%totframes
	np.savetxt('%s.colhist'%sys.argv[1],h/totframes,header=headstring)
	np.savetxt('%s.activehist'%sys.argv[1],s,header=headstring)
	np.savetxt('%s.sumsig'%sys.argv[1],sumimg,header=headstring)
	return

if __name__ == '__main__':
	main()

