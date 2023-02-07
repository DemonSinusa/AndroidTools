# AndroidTools (linux)

1.TerMinimalBootEdit
Readme(eng)
Place the actual boot.img | & | recovery.img file next to the application and run it.
You get a working directory at the output, on subsequent launches the original images will be rebuilt from it.

Create&Work With:

WORK_DIR/

	zImageLoader.bin		//zImage.gz Loader if exists
	zImage.gz				//no comments
	zImageDTB.bin			//Strings table of zImage.gz
	ramdisk/				//Unpacked ramdisk.cpio.gz any users and access rights will be adjusted
	ramdiskLoader.bin		//ramdisk.cpio.gz Loader if exists
	ramdisk.cpio.gz			//no comments
	secondpartLoader.bin	//secondpart.dat Loader if exists
	secondpart.dat			//In official google doc's if have, then jump to second_addr instead kernel
	DevTree.dat				//If bootheader_version contain size (version is 0)
	dtbo.dat				//used in boot v1&&v2
	dtb.dat					//used in boot v2 only
	Orig&FakenowSign.sigh	//Maked self or old paste signature to boot.img
	config.conf				//actual config to Done-Generate boot.img

Modification:

droid_fs_repacker: if there is a "ramdisk /" directory in the working directory, the application will try to assemble ramdisk.cpio
	from it which is converted to ramdisk.cpio.gz using the 
	"$ gzip ramdisk.cpio" which will be packed into the image.
	
zImage_builder_helper:in the presence of a suitable, untreated, nuclear core extract 
	-attach as zImage.pck and exult, then remove the excess
	
Experimental parms:
"CPIO_PACK_BLOCK_ALIGN=0xHEX"-Alignment of ramdisk.cpio archive size generative from ramdisk/.

Out from standarts and limitless.. Good luck.
