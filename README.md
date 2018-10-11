# AndroidTools (linux)

1.TerMinimalBootEdit
Readme(eng)
Place the actual boot.img | & | recovery.img file next to the application and run it.
You get a working directory at the output, on subsequent launches the original images will be rebuilt from it.

Modification: if there is a "ramdisk /" directory in the working directory, the application will try to assemble ramdisk.cpio
 from it which is converted to ramdisk.cpio.gz using the 
 "$ gzip ramdisk.cpio" which will be packed into the image when the application is restarted.
Good luck.