# FGACFS
A Fine-Grained Access Control File System / Folder Sharing

Based on original work FTFS by Roman Dayneko (https://github.com/dayneko3000/Diploma and https://github.com/dayneko3000/prm-control).

tools:
    libfgacfs	FGACFS routines library
    mkfgacfs	utility to create FGACFS filesystem
    fgacfsctl	tool to control FGACFS permissions
    fgacfs	FGACFS fuse driver (mount FGACFS filesystem)

req:		fuse, sqlite
buildreq:	fuse-devel, sqlite-devel

information about fine permissions could be found at
    fgacfsctl --help
