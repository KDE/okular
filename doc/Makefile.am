
# add here all files
data_DATA = index.html index-1.html index-2.html index-3.html index-4.html \
	    index-5.html index-6.html index-7.html index-8.html index-9.html \
	    logotp3.gif

# this is the directory, where all datas are installed
# you have given the datas in data_DATA
datadir = $(kde_htmldir)/en/kdvi

EXTRA_DIST = $(data_DATA) index.sgml index.nif

doc: index.sgml index.nif
	ksgml2html index en
