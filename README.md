# AndroidTools (linux)

1.TerMinimalBootEdit
Readme(eng)
Place the actual boot.img | & | recovery.img file next to the application and run it.
You get a working directory at the output, on subsequent launches the original images will be rebuilt from it.

Create&Work With:
WORK_DIR/
--zImageLoader.bin		//zImage.gz Loader if exists
--zImage.gz				//no comments
--zImageDTB.bin			//Strings table of zImage.gz
--ramdisk/				//Unpacked ramdisk.cpio.gz any users and access rights will be adjusted
--ramdisk.cpio.gz		//no comments
--secondpart.dat		//In official google doc's if have, then jump to second_addr instead kernel
--dtbo.dat				//used in boot v1 only

Modification:
droid_fs_repacker: if there is a "ramdisk /" directory in the working directory, the application will try to assemble ramdisk.cpio
	from it which is converted to ramdisk.cpio.gz using the 
	"$ gzip ramdisk.cpio" which will be packed into the image.
zImage_builder_helper:in the presence of a suitable, untreated, nuclear core extract 
	-attach as zImage.pck and exult, then remove the excess
Good luck.
