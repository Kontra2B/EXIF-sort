A linux tool to distribute pictures based on original EXIF date and to find duplicates based on the date.\

To build run `git clone git@github.com:Kontra2B/EXIF-sort.git`\
Enter checked folder with `cd EXIF-sort` and run `make`.

For help run: `./exif.sort -h`\
Example of picture re-distribution in folder sorted based on original picture EXIF year\
`> ./exif.sort sorted -rYR`
```
Working directory[sorted] recursive, sorting:/yyyy/, pid:2206464
MOVING files: pictures

Press ENTER to continue...
1. sorted/2022/07/07/IMG_20220707_122243.jpg     'M2010J19SY' 2022:07:07-12:22:46 3363868 1506/3264/0...        sorted/2022/IMG_20220707_122243.jpg
2. sorted/2022/12/07/IMG_20221207_154630.jpg	 'M2010J19SY' 2022:12:07-15:46:31 3403885 1840/4000/1... 	sorted/2022/IMG_20221207_154630.jpg
3. sorted/2023/04/02/IMG_20230402_161627.jpg	 'M2010J19SY' 2023:04:02-16:16:27 2813740 4000/1840/1... 	sorted/2023/IMG_20230402_161627.jpg
4. sorted/2020/10/08/DSCF6734.JPG	 'X-A20' 2020:10:08-16:09:07 2497551 4896/3264/1... 	sorted/2020/DSCF6734.JPG
```
