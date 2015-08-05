The ImageSearchDLL exported One function:
char* ImageSearch(
	int aLeft, 
	int aTop, 
	int aRight, 
	int aBottom, 
	char *aImageFile, 
	HBITMAP hbitmap_search);
	
[parameters]:
Here aLeft,aTop,aRight and aBottom specify the area to search in screen.
If you want to search in whole screen, you can set that parameters to (0,0,screen_width,screen_height)

aImageFile specify the path of the image to be searched in harddisk.  More, you can add some instruction before the path to use some advanced function. The mostly useful is specify the tolerance like "*tolerance_number your_path_string", for examble "*32 C:\\test.bmp". tolerance_number are ranged from 0 to 255. 

If you want to search a picture that you already have it's HANDLE,you can send this HANDLE to hbitmap_search, in this way, you don't need specify the image's path by aImageFile.

[result]:
As this dll is write for AutoIt3 to search button postion in screen, inorder to call the function convenient with AutoIt3 scripts, the function result a string.
If search failed, the function will return a string which noly content a char 0.("0")
If search successed，the functhon will return a string that begin with 1, Followed 4 numbers are specify image's xPos,yPos，width，height. Like "1|305|776|25|25"


