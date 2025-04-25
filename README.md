A linux tool to distribute pictures based on original EXIF date and to find duplicates based on the date.\
When moving files some logic makes assertions about overwriting target file.

To build run `git clone git@github.com:Kontra2B/EXIF-sort.git`\
Enter checked folder with `cd EXIF-sort` and run `make`.

For help run: `./exif.sort -h`\
Example of picture re-distribution in the current folder based on original picture EXIF year/month (default):\
`> exif.sort -rR`
```
Working directory[.] recursive, sorting:/yyyy/mm/, pid:2302352
MOVING files: pictures

  Press ENTER to continue...

1. ./2023/03/DSCF7150_2.JPG      'X-A20' 2023:03:05-14:10:15 5779491 4896/3264/1        on place
2. ./2023/03/DSCF7150.JPG	 'X-A20' 2023:03:05-14:10:15 5779491 4896/3264/1	on place
3. ./2023/03/IMG_20230305_141015.jpg	 'M2010J19SY' 2023:03:05-14:10:15 3125647 4000/1840/1	on place
4. ./2023/05/DSCF9018.JPG	 'X-A20' 2023:05:26-18:44:01 3331842 4896/3264/8	on place
5. ./2023/05/DSCF9017.JPG	 'X-A20' 2023:05:26-18:43:54 3362311 4896/3264/1	on place
6. ./2023/05/DSCF9019.JPG	 'X-A20' 2023:05:26-19:33:01 5091778 4896/3264/1	on place
7. ./2023/05/26/DSCF9020.JPG	 'X-A20' 2023:05:26-19:33:06 4475469 4896/3264/1... 	./2023/05/DSCF9020.JPG
8. ./2023/05/28/DSCF9022.JPG	 'X-A20' 2023:05:28-13:39:34 4820136 4896/3264/1... 	./2023/05/DSCF9022.JPG
9. ./2023/05/28/DSCF9021.JPG	 'X-A20' 2023:05:28-13:36:17 4008353 4896/3264/1... 	./2023/05/DSCF9021.JPG
10. ./2023/05/28/DSCF9023.JPG	 'X-A20' 2023:05:28-13:39:44 3484192 4896/3264/8... 	./2023/05/DSCF9023.JPG
11. ./2023/05/28/DSCF9024.JPG	 'X-A20' 2023:05:28-13:39:49 5792150 4896/3264/1... 	./2023/05/DSCF9024.JPG
12. ./2023/05/28/DSCF9025.JPG	 'X-A20' 2023:05:28-13:53:34 4922705 4896/3264/1... 	./2023/05/DSCF9025.JPG
13. ./2023/05/DSCF9016.JPG	 'X-A20' 2023:05:26-18:43:24 3219044 4896/3264/8	on place
14. ./2014/05/31/R0036004.JPG	 'Caplio GX100' 2014:05:31-14:59:34 3684572 3648/2736/1... 	./2014/05/R0036004.JPG
```
